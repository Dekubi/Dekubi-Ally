<!--
# SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
-->

# Dekubi Ally

Dekubi Ally is an experimental, open-source emulator, tuned for **green hat boy** on the **ROG Ally Z1 Extreme** and on PCs with 4 GB VRAM / 8 GB RAM and up. It is a fork of Fritchii / Lichii, which build on Eglantine, Eden, yuzu and Sudachi.

This is an independent hobby fork, not an official release of those projects, and it is not affiliated with big red. I'm new to this, so expect some bugs; I'll do my best to fix them.

## Downloads

Builds are on the [Releases page](https://github.com/Dekubi/Dekubi-Ally/releases). Extract the whole archive and run **dekubi.exe** (Windows x64).

Bring your own files. They are not included, and only legally obtained copies should be used.

Compatibility and performance vary by game, hardware and driver. Rendering errors, stutters and crashes are still possible.

## Reporting a bug

Open an [issue](https://github.com/Dekubi/Dekubi-Ally/issues) with:

- the Dekubi version, game version, CPU, GPU and driver version;
- your settings and the steps to reproduce;
- a screenshot and the log from that session (`user/log/eden_log.txt`), after checking it for personal information.

Do not upload any files or account information except your log and specs.

## Building

See the [build guide](docs/Build.md) and the [coding guidelines](docs/Coding.md). Some build target and file names still use upstream names (the GUI target is `dekubi`, built from `src/yuzu`).

## Credits

Dekubi Ally stands on the work of the Lichii / Fritchii, Eglantine, Eden, yuzu and Sudachi contributors. Built-in Ocarina of Time patches credit their authors in `src/core/file_sys/patch_manager.cpp`. Icon credits: `ICON_CREDITS.html` in the release archive.

Performance optimization, device-tier profiling, memory diagnosis, and upstream porting assisted by Claude (Anthropic) and Gemini (Google DeepMind).

## License

GPL-3.0-or-later, see [LICENSE](LICENSE) and [LICENSES](LICENSES).
