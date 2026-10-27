# HCPS1

HCPS1 is an experimental PlayStation core for **TreeFrogUI picoarch on MIPS32r2 SF3000 / R36SX-class hardware**, including the R36S-labelled V2.6 clone. It builds a complete `hcps1_libretro.so` from pinned PCSX-ReARMed, Lightrec and GNU Lightning sources plus the HCPS1 code in this repository.

**Goal:** good compatibility with roughly 90% of PS1 games, with minimal visual errors. **That is a target, not a measured result.** No retail games or physical handheld have been tested by this project yet. Automated tests are deliberately separate from game compatibility and real-device speed claims. This is a first engineering build, not a promise of full speed in every demanding game.

## What is implemented

- MIPS-to-MIPS dynamic recompilation through Lightrec and GNU Lightning, with the function-restart delay-slot fix and Linux instruction-cache flush fix identified by TreeFrogUI.
- A new non-destructive direct memory mapper for old Linux kernels: four shared PS1 RAM mirrors, exact BIOS/scratchpad offsets, an 8 MiB executable cache, and a RAM-filesystem fallback when `memfd_create` is unavailable. Every allocation is checked and partially completed maps are rolled back.
- A packed integer BGR555-to-RGB565 output converter for this non-NEON CPU. It handles odd widths and unaligned input without the old generic fallback rounding away source pixels. Exhaustive color and bounds tests accompany it.
- GPU UNAI software rendering at native resolution, preserving blending, lighting, dithering and all scanlines. The upstream compatibility database, CPU timing auto-selection, load delays, CD/XA/CDDA, memory cards, disc switching and save states remain available.
- CHD, CUE/BIN, PBP, M3U and the other disc formats supported by the pinned upstream core.
- Original-toolchain MIPS build, native build, sanitizers, generated PS1 homebrew tests, deterministic save-state replay and MIPS execution under QEMU in CI.

This is an extension of an established emulator, not a new PS1 implementation from scratch. See [NOTICE.md](NOTICE.md) for authors, source revisions and licenses.

## Get a build

Open [Actions](https://github.com/MarsInsanity/HCPS1/actions), select a **successful** build, and download `hcps1-sf3000`. GitHub requires sign-in for workflow artifacts. `hcps1-native-test-build` is for Linux PCs, not the handheld. A successful QEMU test does not validate the handheld's kernel, display driver, audio timing or speed.

## Install with TreeFrogUI

1. Back up the SD card's `cubegm/cores/pcsx_rearmed_libretro.so` on your computer.
2. Copy the MIPS `hcps1_libretro.so` into `cubegm/cores/`. To use TreeFrogUI's existing PS / ps1 / psx mapping, install a **copy** named `pcsx_rearmed_libretro.so` there.
3. Put your own dumped PS1 BIOS in `cubegm/bios/` and your games in the existing PS folder. Use the CUE when a game has multiple tracks; use M3U for multi-disc games. For protected games retain their matching SBI files.
4. Start a game. The core identifies itself as **HCPS1**. In core options, set the values in `profiles/quality.cfg` first. Existing PCSX settings can carry over, so explicitly check blending and lighting are enabled and skipline/fast lighting are disabled.
5. If audio breaks up, try the balanced profile: automatic frameskip with a maximum interval of one and simple audio interpolation. This reduces animation smoothness; it does not improve emulation accuracy. It depends on the frontend's audio-buffer callback.

The profile files are readable option lists, **not automatically installed configs**. Apply the listed values through the frontend's core-options menu. Do not overwrite a complete picoarch config with them. Restore the backed-up core to undo installation. Do not rely on cross-core save-state compatibility; back up saves and use in-game memory-card saves when switching cores.

The retail label alone does not identify the CPU. This build is for the MIPS SF3000 family, not genuine ARM RK3326 R36S hardware.

## Build from source

Linux x86-64 is the build host:

```sh
git clone --recurse-submodules https://github.com/MarsInsanity/HCPS1.git
cd HCPS1
python3 scripts/test.py
python3 scripts/build.py --target native
```

For the handheld, extract the [SF3000 v0.1 SDK](https://github.com/game-de-it/sf3000/releases/tag/sf3000_toolchain_v0.1), then:

```sh
export SDK=/absolute/path/mipsel-buildroot-linux-gnu_sdk-buildroot
export CROSS_COMPILE="$SDK/opt/ext-toolchain/bin/mips-mti-linux-gnu-"
export SYSROOT="$SDK/mipsel-buildroot-linux-gnu/sysroot"
python3 scripts/build.py --target sf3000
```

Builds use isolated copies under `build/`, leave upstream sources unmodified, and produce `dist/<target>/` with a source/flags/SHA256 manifest. They use MIPS32r2, o32, little-endian, hard-float FP32, tuning for the reported 74K pipeline, and no mandatory DSP instructions. Do not substitute a modern glibc toolchain and assume the resulting core runs on the old firmware.

## Measure actual performance

The artifact includes `hcps1-bench` and an original `hcps1-test.exe`. On a device shell:

```sh
./hcps1-bench ./hcps1_libretro.so ./hcps1-test.exe
./hcps1-bench ./hcps1_libretro.so /path/game.cue --benchmark 3600 /path/bios
```

The benchmark is unpaced and counts emulated frames, video callbacks and generated audio samples. It has no interactive input or actual display/audio output, so it is a CPU/core throughput check, **not a gameplay benchmark**. Whole-game validation requires interactive picoarch testing. Use [docs/validation.md](docs/validation.md) to record results. Keep CPU timing on Auto; lowering the emulated clock can cause hangs or alter gameplay.

## Current limitations

No measured game-library compatibility percentage or on-device speedup is claimed. Stock firmware may forbid executable mappings or lack a writable tmpfs; allocation failures are reported rather than replacing another mapping. An 8 MiB cache and shared 2 MiB RAM backing reduce duplication, but total emulator and frontend memory use is higher. HLE BIOS is used for automated homebrew tests only; a real BIOS is recommended for game compatibility. DuckStation code is not included.
