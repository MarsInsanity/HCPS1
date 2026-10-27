#!/usr/bin/env python3
import os
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
out = ROOT / "build/tests"
out.mkdir(parents=True, exist_ok=True)
cc = os.environ.get("CC", "cc")
for name in ("video", "memory"):
    exe = out / ("test_" + name)
    flags = ["-std=c99", "-O2", "-Wall", "-Wextra", "-Werror"]
    if name == "video":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    subprocess.run([cc, *flags, str(ROOT / f"tests/test_{name}.c"),
                    str(ROOT / f"src/{name}.c"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
