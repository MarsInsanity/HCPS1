#!/usr/bin/env python3
"""Build the pinned core in an isolated copy; never patch the submodule."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / "sources.lock.json").read_text())

def run(args, **kw):
    print("+", shlex.join(map(str, args)), flush=True)
    return subprocess.run(list(map(str, args)), check=True, **kw)

def replace_once(path, old, new):
    text = path.read_text()
    if text.count(old) != 1:
        raise RuntimeError(f"Patch context mismatch: {path}")
    path.write_text(text.replace(old, new), newline="\n")

def prepare(target):
    source = ROOT / "upstream/pcsx_rearmed"
    for folder, key in [(source, "pcsx_rearmed"),
                        (source / "deps/lightning", "lightning"),
                        (source / "deps/lightrec", "lightrec")]:
        sha = subprocess.check_output(["git", "-C", str(folder), "rev-parse", "HEAD"],
                                      text=True).strip()
        if sha != LOCK[key]["commit"]:
            raise RuntimeError(f"{key}: expected {LOCK[key]['commit']}, got {sha}")
    output = ROOT / "build" / target
    # Only remove this script's generated directory, after resolving it.
    if output.exists():
        if output.is_symlink() or output.resolve().parent != (ROOT / "build").resolve():
            raise RuntimeError("Unsafe build directory")
        shutil.rmtree(output)
    shutil.copytree(source, output,
                    ignore=shutil.ignore_patterns(".git", "*.o", "*.d", "*.so", "*.dll"))
    shutil.copytree(ROOT / "src", output / "hcps1")
    jit = output / "deps/lightning/lib/jit_mips.c"
    replace_once(jit, "patch_alist(1);\n\t\t    goto restart_function;",
                 "patch_alist(1);\n\t\t    /* Drop stale MIPS delay-slot emission on restart. */\n"
                 "\t\t    _jitc->inst.pend = 0;\n\t\t    goto restart_function;")
    replace_once(jit, "_flush_cache((void *)f, t - f, ICACHE);",
                 "__builtin___clear_cache((char *)f, (char *)t);")
    replace_once(output / "frontend/libretro.c",
                 'info->library_name     = "PCSX-ReARMed";',
                 'info->library_name     = "HCPS1";')
    replace_once(output / "frontend/libretro.c",
                 'info->library_version  = "r26" GIT_VERSION;',
                 'info->library_version  = "0.1.0 (PCSX r26)" GIT_VERSION;')
    (output / "Makefile.hcps1").write_text(
        "OBJS += hcps1/video.o hcps1/memory.o\n"
        "CFLAGS += -DHAVE_bgr555_to_rgb565\n"
        "include Makefile.libretro\n", newline="\n")
    return output

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--target", choices=["native", "sf3000"], default="native")
    p.add_argument("--cross", default=os.environ.get("CROSS_COMPILE", ""))
    p.add_argument("--sysroot", default=os.environ.get("SYSROOT", ""))
    p.add_argument("--jobs", type=int, default=min(os.cpu_count() or 2, 8))
    p.add_argument("--prepare-only", action="store_true")
    a = p.parse_args()
    if sys.platform != "linux":
        p.error("Build on Linux (or Linux CI); SF3000's SDK is a Linux toolchain")
    if a.target == "sf3000" and (not a.cross or not a.sysroot):
        p.error("sf3000 requires --cross/PREFIX and --sysroot/PATH (or environment)")
    if a.jobs < 1:
        p.error("--jobs must be positive")
    out = prepare(a.target)
    if a.prepare_only:
        return
    flags = ["-fPIC", "-fno-strict-aliasing", "-fno-fast-math"]
    if a.target == "sf3000":
        flags += ["-EL", "-march=mips32r2", "-mtune=74kc", "-mabi=32",
                  "-mhard-float", "-mfp32", "-mlong-calls", "--sysroot=" + a.sysroot]
    compilers = {}
    for kind, compiler in [("cc", "gcc"), ("cxx", "g++")]:
        exe = shutil.which(a.cross + compiler)
        if not exe:
            p.error("Compiler not found: " + a.cross + compiler)
        wrapper = out / ("hcps1-" + kind)
        wrapper.write_text("#!/bin/sh\nexec " + shlex.quote(exe) + ' "$@" ' +
                           shlex.join(flags) + "\n", newline="\n")
        wrapper.chmod(0o755)
        compilers[kind] = wrapper
    arguments = [
        "make", "-C", out, "-f", "Makefile.hcps1", "-j", str(a.jobs),
        "platform=unix", "DYNAREC=lightrec", "BUILTIN_GPU=unai",
        "LIGHTREC_CUSTOM_MAP=1", "LIGHTREC_CUSTOM_MAP_OBJ=hcps1/lightrec_map.o",
        "LIGHTREC_THREADED_COMPILER=0", "HAVE_NEON_ASM=0", "HAVE_PHYSICAL_CDROM=0",
        "USE_ASYNC_GPU=0", "USE_ASYNC_SPU=0", "USE_ASYNC_CDROM=1",
        "USE_LIBRETRO_VFS=1", "CFLAGS_OPT=-O3", "TARGET=hcps1_libretro.so",
        "CC=" + str(compilers["cc"]), "CXX=" + str(compilers["cxx"]),
        "CC_LINK=" + str(compilers["cxx"]), "CC_AS=" + str(compilers["cc"]),
        "AR=" + a.cross + "ar", "LD=" + a.cross + "ld",
    ]
    if a.target == "sf3000":
        arguments += ["ARCH=mips"]
    run(arguments)
    dest = ROOT / "dist" / a.target
    dest.mkdir(parents=True, exist_ok=True)
    binary = dest / "hcps1_libretro.so"
    shutil.copy2(out / binary.name, binary)
    run([a.cross + "strip", "--strip-unneeded", binary])
    report = {
        "version": "0.1.0", "target": a.target, "sources": LOCK,
        "core_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "compiler": subprocess.check_output([str(compilers["cc"]), "--version"],
                                            text=True).splitlines()[0],
        "flags": flags, "make_arguments": list(map(str, arguments)),
        "hardware_tested": False,
    }
    (dest / "build-manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    for name in ["README.md", "COPYING", "NOTICE.md"]:
        shutil.copy2(ROOT / name, dest / name)
    shutil.copytree(ROOT / "profiles", dest / "profiles", dirs_exist_ok=True)
    print(binary)

if __name__ == "__main__":
    main()
