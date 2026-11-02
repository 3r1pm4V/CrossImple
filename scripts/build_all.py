#!/usr/bin/env python3
"""
Build script chuan cua CrossImple:
Luon tach rieng 2 luong kien truc de tranh xung dot SCons / sdkconfig giua:
  - Stream 1 (ESP32-C3 / RISC-V + custom_sdkconfig): default (Xteink X3)
  - Stream 2 (ESP32-S3 / Xtensa prebuilt libs):      x4-classic, x4-pro
"""

import os
import subprocess
import sys
from pathlib import Path

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")


def find_pio_executable() -> str:
    home = Path.home()
    candidates = [
        home / ".platformio" / "penv" / "Scripts" / "pio.exe",
        home / ".platformio" / "penv" / "bin" / "pio",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return str(candidate)
    return "pio"


def run_step(title: str, cmd: list[str], cwd: Path, env: dict[str, str]) -> None:
    print(f"\n{'=' * 72}")
    print(f"[CrossImple Build] {title}")
    print(f"Command: {' '.join(cmd)}")
    print(f"{'=' * 72}\n", flush=True)
    result = subprocess.run(cmd, cwd=str(cwd), env=env, check=False)
    if result.returncode != 0:
        print(f"\n[ERROR] Step '{title}' failed with exit code {result.returncode}.", file=sys.stderr)
        sys.exit(result.returncode)


def main() -> None:
    project_dir = Path(__file__).resolve().parent.parent
    pio_exe = find_pio_executable()

    env = os.environ.copy()
    if "--export-bin" in sys.argv or env.get("CROSSIMPLE_EXPORT_BIN") == "1":
        env["CROSSIMPLE_EXPORT_BIN"] = "1"

    # Stream 1: Isolated build for Xteink X3 (ESP32-C3 RISC-V, custom_sdkconfig)
    run_step(
        "Stream 1/2 - Building Xteink X3 (ESP32-C3 / env:default)",
        [pio_exe, "run", "-e", "default"],
        project_dir,
        env,
    )

    # Stream 2: Isolated build for Xteink X4 family (ESP32-S3 Xtensa: X4 Classic & X4 Pro)
    run_step(
        "Stream 2/2 - Building Xteink X4 Classic & X4 Pro (ESP32-S3)",
        [pio_exe, "run", "-e", "x4-classic", "-e", "x4-pro"],
        project_dir,
        env,
    )

    print(f"\n{'=' * 72}")
    print("[CrossImple Build] Completed both isolated build streams (X3 + X4 Classic + X4 Pro)!")
    print(f"{'=' * 72}")


if __name__ == "__main__":
    main()
