#include "Imu.h"

#include <BoardConfig.h>

#if FREEINK_CAP_IMU

#include <I2cBusLock.h>
#include <Wire.h>
#include <soc/soc_caps.h>

namespace freeink {
namespace {

// QMI8658 register map (only IMU chip used on Xteink X3 & X4 Classic).
constexpr uint8_t QMI8658_REG_WHO_AM_I = 0x00;
constexpr uint8_t QMI8658_WHO_AM_I_VALUE = 0x05;
constexpr uint8_t QMI8658_REG_CTRL1 = 0x02;
constexpr uint8_t QMI8658_REG_CTRL2 = 0x03;
constexpr uint8_t QMI8658_REG_CTRL3 = 0x04;
constexpr uint8_t QMI8658_REG_CTRL7 = 0x08;
constexpr uint8_t QMI8658_REG_AX_L = 0x35;
constexpr uint8_t QMI8658_REG_GX_L = 0x3B;
constexpr uint8_t QMI8658_ADDR_6A = 0x6A;
constexpr uint8_t QMI8658_ADDR_6B = 0x6B;
constexpr uint8_t QMI8658_CTRL1_AUTO_INC = 1U << 6;
constexpr uint8_t QMI8658_CTRL1_SENSOR_DISABLE = 1U << 0;
constexpr uint8_t QMI8658_CTRL1_BASE = QMI8658_CTRL1_AUTO_INC;
constexpr uint8_t QMI8658_CTRL2_FS_2G = 0U << 4;
constexpr uint8_t QMI8658_CTRL2_ODR_117HZ = 0x06;
constexpr uint8_t QMI8658_CTRL3_FS_512DPS = 0b101U << 4;
constexpr uint8_t QMI8658_CTRL3_ODR_117HZ = 0x06;
constexpr uint8_t QMI8658_CTRL7_ACC_GYRO_ENABLE = 0x03;
constexpr uint8_t QMI8658_CTRL7_DISABLE_ALL = 0x00;
constexpr float QMI8658_ACCEL_G_PER_LSB = 1.0f / 16384.0f;  // +-2 g
constexpr float QMI8658_GYRO_DPS_PER_LSB = 1.0f / 64.0f;    // +-512 dps

bool g_wireReady[2] = {false, false};

TwoWire& sensorWire() {
#if SOC_I2C_NUM > 1
  const auto& s = BoardConfig::ACTIVE.sensors;
  return s.i2cBus == 1 ? Wire1 : Wire;
#else
  return Wire;
#endif
}

void ensureWire() {
  const auto& s = BoardConfig::ACTIVE.sensors;
  const uint8_t bus =
#if SOC_I2C_NUM > 1
      s.i2cBus == 1 ? 1 : 0;
#else
      0;
#endif
  if (g_wireReady[bus]) return;
  if (s.i2cSda < 0 || s.i2cScl < 0) return;
  auto& wire = sensorWire();
  wire.begin(s.i2cSda, s.i2cScl, s.i2cHz);
  g_wireReady[bus] = true;
}

bool writeReg(uint8_t addr, uint8_t reg, uint8_t value) {
  const freeink::I2cBusLock busLock;
  ensureWire();
  auto& wire = sensorWire();
  wire.beginTransmission(addr);
  wire.write(reg);
  wire.write(value);
  return wire.endTransmission() == 0;
}

bool readRegs(uint8_t addr, uint8_t reg, uint8_t* dst, uint8_t len) {
  const freeink::I2cBusLock busLock;
  ensureWire();
  auto& wire = sensorWire();
  wire.beginTransmission(addr);
  wire.write(reg);
  if (wire.endTransmission(false) != 0) return false;
  if (wire.requestFrom(addr, len, static_cast<uint8_t>(true)) < len) return false;
  for (uint8_t i = 0; i < len; ++i) dst[i] = wire.read();
  return true;
}

bool qmi8658PresentAt(uint8_t addr) {
  uint8_t who = 0;
  return readRegs(addr, QMI8658_REG_WHO_AM_I, &who, 1) && who == QMI8658_WHO_AM_I_VALUE;
}

bool powerDownQmi8658(uint8_t addr) {
  const bool sensorsDisabled = writeReg(addr, QMI8658_REG_CTRL7, QMI8658_CTRL7_DISABLE_ALL);
  const bool oscillatorDisabled = writeReg(addr, QMI8658_REG_CTRL1, QMI8658_CTRL1_BASE | QMI8658_CTRL1_SENSOR_DISABLE);
  return sensorsDisabled && oscillatorDisabled;
}

}  // namespace

bool Imu::begin() {
  begun_ = false;
  sleeping_ = false;
  configured_ = false;
  addr_ = 0;

  const auto& s = BoardConfig::ACTIVE.sensors;
  const uint8_t configuredAddr = s.imuAddr;
  if (configuredAddr == 0 || s.i2cSda < 0 || s.i2cScl < 0 || s.i2cHz == 0) return false;
  if (s.imuType != BoardConfig::ImuType::Qmi8658) return false;

  ensureWire();
  const uint8_t alternateAddr = configuredAddr == QMI8658_ADDR_6A ? QMI8658_ADDR_6B : QMI8658_ADDR_6A;
  if (qmi8658PresentAt(configuredAddr)) {
    addr_ = configuredAddr;
  } else if (qmi8658PresentAt(alternateAddr)) {
    addr_ = alternateAddr;
  } else {
    return false;
  }

  const bool configured = writeReg(addr_, QMI8658_REG_CTRL7, QMI8658_CTRL7_DISABLE_ALL) &&
                          writeReg(addr_, QMI8658_REG_CTRL1, QMI8658_CTRL1_BASE) &&
                          writeReg(addr_, QMI8658_REG_CTRL2, QMI8658_CTRL2_FS_2G | QMI8658_CTRL2_ODR_117HZ) &&
                          writeReg(addr_, QMI8658_REG_CTRL3, QMI8658_CTRL3_FS_512DPS | QMI8658_CTRL3_ODR_117HZ) &&
                          writeReg(addr_, QMI8658_REG_CTRL7, QMI8658_CTRL7_ACC_GYRO_ENABLE);
  if (!configured) {
    powerDownQmi8658(addr_);
    return false;
  }

  begun_ = true;
  return true;
}

bool Imu::hasGyroscope() const {
  return begun_ && BoardConfig::ACTIVE.sensors.imuType == BoardConfig::ImuType::Qmi8658;
}

bool Imu::read(Sample& out) {
  const uint8_t addr = addr_;
  if (!begun_ || addr == 0) return false;
  const auto& s = BoardConfig::ACTIVE.sensors;
  if (s.imuType != BoardConfig::ImuType::Qmi8658) return false;

  uint8_t g[6] = {};
  uint8_t a[6] = {};
  if (!readRegs(addr, QMI8658_REG_GX_L, g, sizeof(g))) return false;
  if (!readRegs(addr, QMI8658_REG_AX_L, a, sizeof(a))) return false;

  const int16_t gx = static_cast<int16_t>(g[0] | g[1] << 8);
  const int16_t gy = static_cast<int16_t>(g[2] | g[3] << 8);
  const int16_t gz = static_cast<int16_t>(g[4] | g[5] << 8);
  const int16_t ax = static_cast<int16_t>(a[0] | a[1] << 8);
  const int16_t ay = static_cast<int16_t>(a[2] | a[3] << 8);
  const int16_t az = static_cast<int16_t>(a[4] | a[5] << 8);

  out.ax = ax * QMI8658_ACCEL_G_PER_LSB;
  out.ay = ay * QMI8658_ACCEL_G_PER_LSB;
  out.az = az * QMI8658_ACCEL_G_PER_LSB;
  out.gx = gx * QMI8658_GYRO_DPS_PER_LSB;
  out.gy = gy * QMI8658_GYRO_DPS_PER_LSB;
  out.gz = gz * QMI8658_GYRO_DPS_PER_LSB;

  if (s.imuSwapXY) {
    float t = out.ax;
    out.ax = out.ay;
    out.ay = t;
    t = out.gx;
    out.gx = out.gy;
    out.gy = t;
  }
  if (s.imuFlipX) {
    out.ax = -out.ax;
    out.gx = -out.gx;
  }
  if (s.imuFlipY) {
    out.ay = -out.ay;
    out.gy = -out.gy;
  }
  return true;
}

bool Imu::sleep() {
  const uint8_t addr = addr_;
  if (!begun_ || addr == 0) return false;
  if (BoardConfig::ACTIVE.sensors.imuType != BoardConfig::ImuType::Qmi8658) return false;
  return powerDownQmi8658(addr);
}

bool Imu::wake() {
  const uint8_t addr = addr_;
  if (!begun_ || addr == 0) return false;
  if (BoardConfig::ACTIVE.sensors.imuType != BoardConfig::ImuType::Qmi8658) return false;
  return writeReg(addr, QMI8658_REG_CTRL1, QMI8658_CTRL1_BASE) &&
         writeReg(addr, QMI8658_REG_CTRL7, QMI8658_CTRL7_ACC_GYRO_ENABLE);
}

}  // namespace freeink

#else  // FREEINK_CAP_IMU — IMU absent.

namespace freeink {
bool Imu::begin() { return false; }
bool Imu::hasGyroscope() const { return false; }
bool Imu::read(Sample&) { return false; }
bool Imu::sleep() { return false; }
bool Imu::wake() { return false; }
}  // namespace freeink

#endif  // FREEINK_CAP_IMU
