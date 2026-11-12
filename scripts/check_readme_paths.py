"""Check that every backtick path in README.md's Feature Attribution table exists in the repository."""

from __future__ import annotations

import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
README_PATH = os.path.join(REPO_ROOT, "README.md")
HEADER = "## 🧭 Nguồn gốc các tính năng"


def extract_attribution_paths(readme_text: str) -> list[tuple[int, str]]:
    lines = readme_text.splitlines()
    in_section = False
    paths: list[tuple[int, str]] = []
    for lineno, line in enumerate(lines, start=1):
        if line.startswith(HEADER):
            in_section = True
            continue
        if in_section:
            if line.startswith("## ") or line.strip() == "---":
                if paths:
                    break
                continue
            if not line.lstrip().startswith("|"):
                continue
            cols = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cols) < 3 or cols[0].startswith("---") or "Tệp tiêu biểu" in cols[2]:
                continue
            for match in re.finditer(r"`([^`]+)`", cols[2]):
                candidate = match.group(1).strip()
                if "/" in candidate or candidate.endswith((".cpp", ".h", ".py", ".ini", ".txt", ".md")):
                    paths.append((lineno, candidate))
    return paths


def main() -> int:
    with open(README_PATH, encoding="utf-8") as f:
        text = f.read()

    paths = extract_attribution_paths(text)
    if not paths:
        print("ERROR: No paths found in Feature Attribution table.", file=sys.stderr)
        return 2

    missing: list[tuple[int, str]] = []
    for lineno, rel in paths:
        full = os.path.join(REPO_ROOT, rel.rstrip("/"))
        exists = os.path.exists(full)
        status = "OK" if exists else "MISSING"
        print(f"[{status:7s}] L{lineno}: {rel}")
        if not exists:
            missing.append((lineno, rel))

    print(f"\nChecked {len(paths)} path(s): {len(paths) - len(missing)} OK, {len(missing)} MISSING.")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
