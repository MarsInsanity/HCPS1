# Sources and attribution

HCPS1 additions are GPL-2.0-or-later. The complete core is built from PCSX-ReARMed and its dependencies, whose individual notices and licenses are retained in the pinned submodules.

- PCSX-ReARMed: https://github.com/notaz/pcsx_rearmed at 8b6c163575e6ce9a0537553e0d4ed4225315955e. Principal project by Gražvydas “notaz” Ignotas and contributors; inherits PCSX, PCSX-df, PCSX-Reloaded and PCSX-Revolution. GPU UNAI and SPU authors retain their original notices. See upstream AUTHORS and COPYING.
- Lightrec: Paul Cercueil and contributors, https://github.com/pcercuei/lightrec at 853d0c81f33a7498c9f8ea8a42a19fae48132bb6; LGPL-2.1-or-later.
- GNU Lightning: https://github.com/notaz/gnu_lightning at a6bb2b5a7cf36e074e12ccaed32990b437deb784; its COPYING and source headers apply.
- TreeFrogUI and TreeFrogUI_picoarch by Tomasz Zubertowski and contributors supplied platform research. The two GNU Lightning fixes are adapted from https://github.com/tzubertowski/TreeFrogUI/blob/main/patches/pcsx_rearmed-sf3000-lightrec.patch (blob e40957021fbdaf134a29ee1e273eda25bdb07dfb), which modifies the licensed GNU Lightning implementation. HCPS1 independently implements the memory mapper and does not copy the frontend, UI assets or graphics-degrading defaults.
- SF3000 SDK: https://github.com/game-de-it/sf3000/releases/tag/sf3000_toolchain_v0.1.
- JohnnyonFlame/psx4all-dingoo and stenzek/duckstation were reviewed as architecture references; no code from those repositories is included.

When redistributing binaries, supply the corresponding source: this exact HCPS1 revision, all recursive pinned submodules, build scripts and licenses. GitHub-generated source ZIPs do not include submodule contents; use a recursive checkout. No Sony firmware, BIOS or commercial game data is distributed.
