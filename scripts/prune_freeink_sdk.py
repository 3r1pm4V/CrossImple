"""
PlatformIO pre-build script: skip compiling unrelated FreeInk SDK panel drivers
and replace the generic multi-IMU file (which embeds an 8.5 KB BMI270 firmware
blob) with our QMI8658-only implementation in src/platform/ImuQmi8658.cpp.

Filters `src_filter` in CollectBuildFiles rather than using AddBuildMiddleware,
because on Windows pioarduino wraps middlewares and discards None return values.
"""

import os
from pathlib import Path
from platformio.builder.tools import piobuild

try:
    Import("env")  # noqa: F821
except NameError:
    env = None


def _patch_file_once(path: Path, old: str, new: str) -> None:
    if not path.is_file():
        return
    text = path.read_text(encoding="utf-8")
    if new in text:
        return
    if old in text:
        path.write_text(text.replace(old, new, 1), encoding="utf-8")
        print(f"[prune_freeink_sdk] Patched {path.name}")


def _patch_freeink_sdk(project_dir: Path) -> None:
    sdk_dir = project_dir / "freeink-sdk"
    if not sdk_dir.is_dir():
        return

    # 1. Metalio E-Ink 4: Hold modem MRDY (GPIO21) HIGH at boot
    board_h = sdk_dir / "libs/hardware/BoardConfig/include/MetalioEInk4Board.h"
    _patch_file_once(
        board_h,
        "constexpr uint8_t PIN_POWER_PULSE = 11;\nconstexpr int VIBRATION_GPIO = 44;",
        "constexpr uint8_t PIN_POWER_PULSE = 11;\nconstexpr int MODEM_MRDY_GPIO = 21;\nconstexpr int VIBRATION_GPIO = 44;",
    )
    _patch_file_once(
        board_h,
        "  pinMode(VIBRATION_GPIO, OUTPUT);\n  digitalWrite(VIBRATION_GPIO, LOW);\n  ready = true;",
        "  pinMode(VIBRATION_GPIO, OUTPUT);\n  digitalWrite(VIBRATION_GPIO, LOW);\n  pinMode(MODEM_MRDY_GPIO, OUTPUT);\n  digitalWrite(MODEM_MRDY_GPIO, HIGH);\n  ready = true;",
    )

    # 2. InputManager.h: Expose BTN_EXTRA = 7
    input_h = sdk_dir / "libs/hardware/InputManager/include/InputManager.h"
    _patch_file_once(
        input_h,
        "  static constexpr uint8_t BTN_POWER = 6;\n",
        "  static constexpr uint8_t BTN_POWER = 6;\n  static constexpr uint8_t BTN_EXTRA = 7;\n",
    )

    # 3. InputManager.cpp: CST816S immediate IRQ poll, Lift-Up bezel latch, and Left/Right pill mapping
    input_cpp = sdk_dir / "libs/hardware/InputManager/src/InputManager.cpp"
    _patch_file_once(
        input_cpp,
        """  if (now < touchReadAt) return;
  touchReadAt = now + TOUCH_SAMPLE_DELAY_MS;
  const bool active = touchPressed || touchHomeKeyDown || cstVirtualButtons;
  const bool fresh = cstInterrupt || t.irq < 0 || digitalRead(t.irq) == LOW;
  if (!active && !fresh) return;
  cstInterrupt = false;""",
        """  const bool active = touchPressed || touchHomeKeyDown || cstVirtualButtons;
  if (active && now < touchReadAt) return;
  const bool fresh = cstInterrupt || t.irq < 0 || digitalRead(t.irq) == LOW;
  if (!active && !fresh) return;
  touchReadAt = now + TOUCH_SAMPLE_DELAY_MS;
  cstInterrupt = false;""",
    )
    _patch_file_once(
        input_cpp,
        """  if (!(data[0] & 0x0F) || (data[1] >> 6) == 1) {
    release();
    return;
  }
  const uint16_t rawX = ((data[1] & 0x0F) << 8) | data[2];
  const uint16_t rawY = ((data[3] & 0x0F) << 8) | data[4];
  if (BoardConfig::isMetalioEInk4() && rawY >= 800) {
    // Vendor cover key centers: HOME=(80,900), NEXT=(240,900), PREV=(400,900).
    // Ignore invalid off-panel samples rather than clamping them into screen taps.
    if (rawY < 860 || rawY > 940 || rawX > 479) { release(); return; }
    if (touchPressed) { suppressTouchContact(); release(); }
    if (rawX < 160) {
      cstVirtualButtons = 0;
      if (!touchHomeKeyDown) {
        touchHomeKeyDown = true;
        touchHomeKeyPressedEvent = true;
        touchHomeKeyStart = now;
        touchHomeKeyLongFired = false;
      } else if (!touchHomeKeyLongFired && now - touchHomeKeyStart >= HOME_KEY_LONG_PRESS_MS) {
        touchHomeKeyLongFired = true;
        touchHomeKeyEvent = true;
        touchHomeKeyLongPressEvent = true;
      }
    } else {
      if (touchHomeKeyDown) release();
      cstVirtualButtons = (1 << (rawX < 320 ? BTN_DOWN : BTN_UP));
    }
    return;
  }""",
        """  const uint16_t rawX = ((data[1] & 0x0F) << 8) | data[2];
  const uint16_t rawY = ((data[3] & 0x0F) << 8) | data[4];
  const bool isUpFrame = !(data[0] & 0x0F) || (data[1] >> 6) == 1;
  if (isUpFrame) {
    // If a quick bezel-key tap completed before the first poll (active == false
    // on a fresh Lift-Up frame), latch that bezel key for one sample window so
    // the tap is not lost.
    if (!active && fresh && (data[1] >> 6) == 1 && BoardConfig::isMetalioEInk4() &&
        rawY >= 840 && rawY <= 980 && rawX <= 479) {
      if (rawX < 160) {
        touchHomeKeyEvent = true;
        touchHomeKeyTapEvent = true;
      } else {
        cstVirtualButtons = (1 << (rawX < 320 ? BTN_UP : BTN_DOWN));
      }
      return;
    }
    release();
    return;
  }
  if (BoardConfig::isMetalioEInk4() && rawY >= 800) {
    // Vendor cover key centers: HOME=(80,900), NEXT=(240,900), PREV=(400,900).
    // Bezel buttons sit strictly in 840..980px range below screen; ignore near-screen margin (800..839px).
    if (rawY < 840 || rawY > 980 || rawX > 479) {
      release();
      return;
    }
    if (touchPressed) {
      suppressTouchContact();
      release();
    }
    if (rawX < 160) {
      cstVirtualButtons = 0;
      if (!touchHomeKeyDown) {
        touchHomeKeyDown = true;
        touchHomeKeyPressedEvent = true;
        touchHomeKeyStart = now;
        touchHomeKeyLongFired = false;
      } else if (!touchHomeKeyLongFired && now - touchHomeKeyStart >= HOME_KEY_LONG_PRESS_MS) {
        touchHomeKeyLongFired = true;
        touchHomeKeyEvent = true;
        touchHomeKeyLongPressEvent = true;
      }
    } else {
      if (touchHomeKeyDown) release();
      // Field guide: middle pill = Page Up (BTN_UP), right pill = Page Down (BTN_DOWN)
      cstVirtualButtons = (1 << (rawX < 320 ? BTN_UP : BTN_DOWN));
    }
    return;
  }""",
    )


def _append_src_filter(current_filter, exclusions):
    if isinstance(current_filter, (list, tuple)):
        return list(current_filter) + list(exclusions)
    base = current_filter.strip() if isinstance(current_filter, str) and current_filter.strip() else "+<*>"
    return base + " " + " ".join(exclusions)


def _prune_freeink_sdk(build_env):
    project_dir = Path(build_env.subst("$PROJECT_DIR"))
    _patch_freeink_sdk(project_dir)
    pioenv = build_env.get("PIOENV", "")

    skip_drivers = [
        "Ed2208M5Driver.cpp",
        "It8951Driver.cpp",
        "LgfxEpdDriver.cpp",
        "M5OfficialDriver.cpp",
        "PaperMonoDriver.cpp",
        "Ssd2677Driver.cpp",
        "Uc8253MurphyDriver.cpp",
        "Uc8279cA4Driver.cpp",
    ]

    if pioenv in ("default", "debug"):
        # Pure X3 target only selects UC8253 or UC8279d.
        skip_drivers.extend(["Ssd1677Driver.cpp", "Uc8179Driver.cpp", "Uc8279X4Driver.cpp"])
    elif pioenv in ("x4-classic", "x4-classic-debug", "x4-pro", "x4-pro-debug"):
        # X4 Classic / X4 Pro only select SSD1677, UC8179 or UC8279 (X4 variant).
        skip_drivers.extend(["Uc8253X3Driver.cpp", "Uc8279Driver.cpp"])

    orig_collect = piobuild.CollectBuildFiles

    def filtered_collect_build_files(target_env, variant_dir, src_dir, src_filter=None, duplicate=False):
        norm_dir = os.path.realpath(target_env.subst(src_dir)).replace("\\", "/").rstrip("/")
        if norm_dir.endswith("/freeink-sdk/libs/display/FreeInkDisplay/src"):
            exclusions = [f"-<driver/{name}>" for name in skip_drivers]
            src_filter = _append_src_filter(src_filter, exclusions)
            print(f"[prune_freeink_sdk] Pruning {len(skip_drivers)} display drivers for {pioenv}")
        elif norm_dir.endswith("/freeink-sdk/libs/hardware/Imu/src"):
            src_filter = _append_src_filter(src_filter, ["-<Imu.cpp>"])
            print(f"[prune_freeink_sdk] Replacing multi-IMU Imu.cpp with ImuQmi8658.cpp for {pioenv}")
        return orig_collect(target_env, variant_dir, src_dir, src_filter, duplicate)

    build_env.AddMethod(filtered_collect_build_files, "CollectBuildFiles")


if env is not None:
    _prune_freeink_sdk(env)
