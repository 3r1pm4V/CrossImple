// RadioPort over the SDK's BleKeyboardHost or ESP-IDF's ble_page_turner,
// with the radio start on a FreeRTOS task.
#if (defined(FREEINK_CAP_BLE_HID_HOST) && FREEINK_CAP_BLE_HID_HOST) || defined(CONFIG_BT_NIMBLE_ENABLED)

#if defined(__has_include) && __has_include(<BleKeyboardHost.h>)
#include <BleKeyboardHost.h>
#define BLE_HAS_KEYBOARD_HOST 1
#else
#define BLE_HAS_KEYBOARD_HOST 0
#include "ble_page_turner.h"
#endif

#include <esp_attr.h>
#include <esp_timer.h>
#include <atomic>
#include "RadioPort.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace bleturner::port {
namespace {

#if BLE_HAS_KEYBOARD_HOST
freeink::BleKeyboardHost& radioHost = freeink::BleKeyboardHost::getInstance();
freeink::BleKeyboardHost& hid() { return radioHost; }
#endif

RTC_NOINIT_ATTR Memo memo;

#ifdef BLE_PAGE_TURNER_STACK_PROBE
std::atomic<uint32_t> startStackMinimum{0};
#endif

void startTaskEntry(void*) {
  detail::startTask();
#ifdef BLE_PAGE_TURNER_STACK_PROBE
  startStackMinimum.store(uxTaskGetStackHighWaterMark(nullptr), std::memory_order_release);
#endif
  vTaskDelete(nullptr);
}

Peer peerOf(const char* addr, const char* name) { return Peer{addr, name}; }

}  // namespace

bool compiledIn() { return true; }

#if BLE_HAS_KEYBOARD_HOST
bool begin() { return hid().begin("FreeInk"); }
bool end(const uint32_t timeoutMs) { return hid().end(timeoutMs); }
bool running() { return hid().isRunning(); }
bool stopping() { return hid().isStopping(); }
bool connected() { return hid().isConnected(); }
bool connecting() { return hid().isConnecting(); }
ConnectionActivity connectionActivity() { return {hid().isConnecting(), 0}; }
bool scanning() { return hid().isScanning(); }
void poll() { hid().poll(); }

bool popRaw(RawEdge& out) {
  freeink::RawButtonEvent ev;
  if (!hid().popRawButton(ev)) return false;
  out.reportId = ev.reportId;
  out.byteIndex = ev.byteIndex;
  out.value = ev.value;
  out.pressed = ev.pressed;
  out.keycode = ev.keycode;
  out.mods = ev.mods;
  out.wasRest = ev.wasRest;
  out.atMs = ev.atMs;
  return true;
}

bool popKey(KeyPress& out) {
  freeink::KeyEvent ev;
  if (!hid().popKey(ev)) return false;
  out.keycode = ev.keycode;
  out.mods = ev.mods;
  out.pressed = ev.pressed;
  return true;
}

bool armBondedReconnect(uint8_t, const char* priorityAddr) {
  return priorityAddr != nullptr && priorityAddr[0] != '\0' && hid().armSelectedPeerReconnect(priorityAddr);
}
Peer linked() { return peerOf(hid().connectedAddr(), hid().connectedName()); }

void scan(const uint32_t durationMs) {
  if (durationMs == 0) {
    hid().stopScan();
  } else {
    hid().startScan(durationMs);
  }
}

bool connect(const char* addr) { return hid().connect(addr); }
void disconnect() { hid().disconnect(); }
void forget(const char* addr) { hid().forget(addr); }
bool takeConnectFailure(char* out, const size_t outLen) { return hid().takeConnectFailure(out, outLen); }
uint8_t bondCount() { return hid().pairedCount(); }
Peer bond(const uint8_t index) { return peerOf(hid().paired(index).addr, hid().paired(index).name); }
uint8_t foundCount() { return hid().deviceCount(); }
Peer found(const uint8_t index) { return peerOf(hid().device(index).addr, hid().device(index).name); }

#else // IDF native ble_page_turner bridge

bool begin() { return ble_pt_start() == ESP_OK; }
bool end(const uint32_t timeoutMs) { return ble_pt_stop(timeoutMs) == ESP_OK; }
bool running() { return ble_pt_running(); }
bool stopping() { return false; }
bool connected() { return ble_pt_connected(); }
bool connecting() { return ble_pt_connecting(); }
ConnectionActivity connectionActivity() { return {ble_pt_connecting(), 0}; }
bool scanning() { return ble_pt_scanning(); }
void poll() { ble_pt_poll(); }

bool popRaw(RawEdge& out) {
  ble_pt_raw_t raw;
  if (!ble_pt_pop_raw(&raw)) return false;
  out.reportId = raw.frame.len > 0 ? raw.frame.bytes[0] : 0;
  out.byteIndex = 0;
  out.value = raw.frame.len > 1 ? raw.frame.bytes[1] : (raw.frame.len > 0 ? raw.frame.bytes[0] : 0);
  out.pressed = raw.pressed;
  out.keycode = 0;
  out.mods = 0;
  out.wasRest = false;
  out.atMs = raw.at_ms;
  return true;
}

bool popKey(KeyPress& out) {
  ble_pt_event_t ev;
  if (!ble_pt_pop_key(&ev)) return false;
  out.keycode = ev.usage;
  out.mods = ev.mods;
  out.pressed = true;
  return true;
}

bool armBondedReconnect(uint8_t, const char* priorityAddr) {
  if (priorityAddr && priorityAddr[0] != '\0') {
    return ble_pt_connect(priorityAddr) == ESP_OK;
  }
  ble_pt_reconnect_recent();
  return true;
}

Peer linked() { return peerOf("", ble_pt_connected_name()); }

void scan(const uint32_t durationMs) {
  if (durationMs == 0) {
    ble_pt_scan_stop();
  } else {
    ble_pt_scan_start(durationMs);
  }
}

bool connect(const char* addr) { return ble_pt_connect(addr) == ESP_OK; }
void disconnect() { ble_pt_disconnect(); }
void forget(const char* addr) { ble_pt_forget(addr); }
bool takeConnectFailure(char* out, const size_t outLen) { return ble_pt_take_failure(out, outLen); }
uint8_t bondCount() { return ble_pt_bond_count(); }
Peer bond(const uint8_t index) {
  const ble_pt_bond_t* b = ble_pt_bond(index);
  return b ? peerOf(b->addr, b->name) : peerOf("", "");
}
uint8_t foundCount() { return ble_pt_device_count(); }
Peer found(const uint8_t index) {
  const ble_pt_device_t* d = ble_pt_device(index);
  return d ? peerOf(d->addr, d->name) : peerOf("", "");
}

#endif

bool spawnStart() { return xTaskCreate(startTaskEntry, "ble-start", 3072, nullptr, 2, nullptr) == pdPASS; }
uint32_t nowMs() { return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL); }
void sleepMs(const uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
Memo& restartMemo() { return memo; }

#ifdef BLE_PAGE_TURNER_PROBE
void inject(const uint8_t* frame, const size_t len) {
#if BLE_HAS_KEYBOARD_HOST
  hid().onReportIngest(frame, len);
#else
  (void)frame; (void)len;
#endif
}
unsigned rawOverflows() {
#if BLE_HAS_KEYBOARD_HOST
  return hid().rawOverflows();
#else
  return 0;
#endif
}
#endif
#ifdef BLE_PAGE_TURNER_STACK_PROBE
uint32_t startStackLeft() { return startStackMinimum.load(std::memory_order_acquire); }
#endif

}  // namespace bleturner::port

#endif
