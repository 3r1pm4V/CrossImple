"""
PlatformIO pre-build script: skip compiling unrelated FreeInk SDK panel drivers
and replace the generic multi-IMU file (which embeds an 8.5 KB BMI270 firmware
blob) with our QMI8658-only implementation in src/platform/ImuQmi8658.cpp.

Filters `src_filter` in CollectBuildFiles rather than using AddBuildMiddleware,
because on Windows pioarduino wraps middlewares and discards None return values.
"""

import os
from platformio.builder.tools import piobuild

try:
    Import("env")  # noqa: F821
except NameError:
    env = None


def _append_src_filter(current_filter, exclusions):
    if isinstance(current_filter, (list, tuple)):
        return list(current_filter) + list(exclusions)
    base = current_filter.strip() if isinstance(current_filter, str) and current_filter.strip() else "+<*>"
    return base + " " + " ".join(exclusions)


def _prune_freeink_sdk(build_env):
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
