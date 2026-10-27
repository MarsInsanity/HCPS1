# Validation and compatibility protocol

## Automated checks

The CI workflow tests all 65,536 BGR555 values, pair independence, source/destination alignments, odd and short spans, and bounds under ASan/UBSan. Memory tests cover partially occupied ranges, rollback, true shared mirrors, tmpfs fallback, repeated allocation/free, and invalid arguments. The generated PS-X EXE tests execution, a branch delay slot, GPU fill, audio callback delivery and deterministic state replay.

The target job uses the original SF3000 SDK and executes the MIPS binaries under qemu-mipsel, including Lightrec JIT execution with forced tmpfs fallback. This is a real target-ISA test but not a physical device test. It cannot establish performance or firmware compatibility. Read the actual workflow result before treating a build as passing.

## Physical device acceptance

Record /proc/cpuinfo, uname -a, firmware version, RAM, TreeFrogUI version, core SHA256, BIOS hash/region, game serial/revision/region, disc format and every changed option. Do not publish game/BIOS data. Confirm the log identifies HCPS1 and its Lightrec memory map.

Check cold boot, loading, 30 minutes of active play, FMV, CDDA/XA, pause/resume, repeated loads, memory-card save/restart/load, save-state save/load, and disc swap for multi-disc titles. Examine transparency, lighting, framebuffer effects, dithering, interlace/high-resolution scenes and audio. Compare screenshots against the same scene on hardware or a trusted accurate emulator. Count emulated speed separately from displayed FPS: frameskip can conceal slowdown.

Use identical game scene, firmware, settings and temperature for stock versus HCPS1. Capture median and low-percentile speed and audio underruns over three runs, including demanding gameplay rather than only an attract screen. The command-line benchmark excludes display/audio device overhead and has no controller input.

## Initial stress set (all currently UNTESTED)

Tekken 3; Gran Turismo 2; Driver 2; Colin McRae Rally 2.0; Vagrant Story; Metal Gear Solid; Final Fantasy IX; Chrono Cross; Crash Team Racing; Spyro 2; Silent Hill; R-Type Delta; Bloody Roar II; Ridge Racer Type 4; Wipeout 3; Parasite Eve II; Soul Reaver; Tony Hawk's Pro Skater 2; Formula One 99; Ape Escape (needs a suitable analog-controller setup).

This list deliberately probes difficult features and is not representative enough to support a library-wide percentage.

## The 90% target

Define the population first (unique games, treatment of regions/revisions), select a representative stratified/random sample across genres and GPU/CD features, predefine pass criteria, and publish numerator, denominator and untested count. Keep separate categories for playable, visual accuracy, near-full-speed, audio, and completion. Booting is not a compatibility pass. A stress set alone does not establish 90% compatibility.

A suggested row format for issue reports:

Game / serial / region; core SHA; firmware; BIOS; settings; scene and duration; emulation-speed measurements; audio underruns; visual defects; save/load; disc swap; reproduction steps; pass/fail/untested.
