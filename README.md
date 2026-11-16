<!--
# SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
-->

# Dekubi Ally

Dekubi Ally is an experimental, open-source Nintendo Switch and Switch 2 emulator, tuned for **The Legend of Zelda: Ocarina of Time** on the **ROG Ally Z1 Extreme** and on PCs with 4 GB VRAM / 8 GB RAM and up. It is a fork of Fritchii / Lichii, which build on Eglantine, Eden, yuzu and Sudachi.

This is an independent hobby fork, not an official release of those projects, and it is not affiliated with Nintendo. I'm new to this, so expect some bugs; I'll do my best to fix them.

## Downloads

Builds are on the [Releases page](https://github.com/Dekubi/Dekubi-Ally/releases). Extract the whole archive and run **dekubi.exe** (Windows x64).

Bring your own games, keys and firmware. They are not included, and only legally obtained copies should be used.

### Shaders

Release archives contain no compiled shaders. On first launch with Ocarina of Time, Dekubi uses the included recipe (`recipes/`) to build the game's ~120,000 pipelines **on your own device**. A side panel shows the progress; you can press **Play now** or close the panel and keep playing while it finishes in the background.

## What's different in Dekubi

- **Ocarina of Time guard-house freeze fixed** (Texture Pool x2 built-in, a port of zortos293/Lichii#1313), also with Max Resolution on.
- **Guard-house frame rate**: texture cache work from zortos293/Lichii#1314 (about 16 → 30–37 fps there while turning the camera on the test PC).
- Movable, closable "Preparing shaders" side panel; custom PNG icons; green performance graph.
- 30 FPS / 60 FPS mode switch and Dynamic FPS (in Add-Ons) for Ocarina of Time.

Compatibility and performance vary by game, hardware and driver. Rendering errors, stutters and crashes are still possible.

## Reporting a bug

Open an [issue](https://github.com/Dekubi/Dekubi-Ally/issues) with:

- the Dekubi version, game version, CPU, GPU and driver version;
- your settings and the steps to reproduce;
- a screenshot and the log from that session (`user/log/eden_log.txt`), after checking it for personal information.

Do not upload game files, keys, firmware or account information.

## Building

See the [build guide](docs/Build.md) and the [coding guidelines](docs/Coding.md). Some build target and file names still use upstream names (the GUI target is `dekubi`, built from `src/yuzu`).

## Credits

Dekubi Ally stands on the work of the Lichii / Fritchii, Eglantine, Eden, yuzu and Sudachi contributors. Built-in Ocarina of Time patches credit their authors in `src/core/file_sys/patch_manager.cpp`. Icon credits: `ICON_CREDITS.html` in the release archive.

## License

GPL-3.0-or-later, see [LICENSE](LICENSE) and [LICENSES](LICENSES).
