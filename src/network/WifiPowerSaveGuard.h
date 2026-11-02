#pragma once

#ifndef SIMULATOR
#include <Logging.h>
#include <esp_wifi.h>
#endif

// Network transfers can run for minutes (OPDS crawls, KOSync calls, OTA
// streams). Modem sleep powers the radio down between DTIM beacons, which adds
// multi-second latency to TLS handshakes and can stall packets mid-transfer,
// so disable WiFi power-save for the duration of the exchange. The previous
// mode is restored on scope exit, so guards nest safely and contexts that keep
// power-save off for their whole lifetime (the web server) are not reverted.
class WifiPowerSaveGuard final {
 public:
#ifndef SIMULATOR
  WifiPowerSaveGuard() {
    if (esp_wifi_get_ps(&previousMode_) != ESP_OK) {
      previousMode_ = WIFI_PS_MIN_MODEM;
    }
    const esp_err_t err = esp_wifi_set_ps(WIFI_PS_NONE);
    if (err != ESP_OK) {
      LOG_ERR("WIFI", "Failed to disable WiFi power-save: %d", err);
    }
  }

  ~WifiPowerSaveGuard() {
    if (previousMode_ == WIFI_PS_NONE) return;
    const esp_err_t err = esp_wifi_set_ps(previousMode_);
    if (err != ESP_OK) {
      LOG_ERR("WIFI", "Failed to restore WiFi power-save: %d", err);
    }
  }
#else
  WifiPowerSaveGuard() = default;
  ~WifiPowerSaveGuard() = default;
#endif

  WifiPowerSaveGuard(const WifiPowerSaveGuard&) = delete;
  WifiPowerSaveGuard& operator=(const WifiPowerSaveGuard&) = delete;

 private:
#ifndef SIMULATOR
  wifi_ps_type_t previousMode_ = WIFI_PS_MIN_MODEM;
#endif
};
