#pragma once

#if !defined(SIMULATOR) && defined(ARDUINO)
#include <Arduino.h>
#include <esp_err.h>
#include <esp_task_wdt.h>

#include <cstdint>

// Ported from paperpaw-main (shells/xteink/src/power.rs: WORK_S = 900s):
// Bound how long worker/IO loops can keep feeding the Task Watchdog without
// returning to the main loop, so a corrupted circular FAT cluster chain cannot
// hang the device indefinitely.
constexpr uint32_t MAX_WDT_WORK_MS = 900000UL;  // 15 minutes

inline volatile uint32_t& watchdogLastMainLoopTickMs() {
  static volatile uint32_t lastTickMs = 0;
  return lastTickMs;
}

inline void markWatchdogMainLoopTick() { watchdogLastMainLoopTickMs() = millis(); }

inline void resetTaskWatchdogIfSubscribed() {
  const uint32_t now = millis();
  const uint32_t lastTick = watchdogLastMainLoopTickMs();
  if (lastTick != 0 && (now - lastTick) > MAX_WDT_WORK_MS) {
    return;
  }
  if (esp_task_wdt_status(nullptr) == ESP_OK) {
    esp_task_wdt_reset();
  }
}
#else
inline void markWatchdogMainLoopTick() {}
inline void resetTaskWatchdogIfSubscribed() {}
#endif
