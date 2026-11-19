# Dekubi Ally v1.1.6 beta

### 🎯 Affected Games & Who Benefits
- **The Legend of Zelda: Ocarina of Time (`0400486024DEC000`):**
  - **Zora River Freeze Fixed:** Completely removes the 6.3-second freeze when entering Zora River and cold compute zones.
  - **Interactive Element Hitches Fixed:** Eliminates up to 1-second freezes when objects like the Deku Tree eye, ladders, torches, and player shields first render.
  - **Split Semaphore Stalls Removed:** Cuts over 3 seconds of cumulative GPU split drain hitches per play session.
  - **Dynamic FPS & High Refresh:** Full compatibility with the 60+ FPS dynamic refresh pipeline without audio or speed desyncs.
  - **Universal Temporal Upscaling:** Seamlessly choose between **AMD FSR 4 / Redstone**, **Intel XeSS**, and **NVIDIA DLSS** (or native rendering).
- **Switch 2 & Modern Mesh Titles:**
  - Games utilizing native mesh shaders and bounded compute dispatch cycles run with seamless pipeline deduplication.
- **All Games on Memory-Constrained Hardware (ROG Ally & 4 GB VRAM / 8 GB RAM PCs):**
  - Reduces host commit footprint by ~5.5 GiB and reclaims idle physical RAM, preventing out-of-memory crashes and severe paging dips across your entire library.

---

### ⚡ Complete Upscaling Trio: XeSS, FSR & DLSS (with DLSS 5 Support)
- **Intel XeSS (AI Super Sampling & Neural Reconstruction):**
  - Added native Intel XeSS Super Resolution and Anti-Aliasing support (`libxess.dll` in `user/xess/`).
  - Brings AI tensor reconstruction and jittered camera anti-aliasing to ROG Ally, Intel Arc, AMD Radeon, and NVIDIA GeForce cards.
  - Dedicated configuration panel with folder shortcut and direct link to Intel's official repository.
- **AMD FSR 4 / Redstone (FidelityFX SDK 2.4.0 ML):**
  - Integrated AMD FidelityFX SDK 2.4.0 (Redstone branch) featuring machine-learning upscaling for RDNA 2, RDNA 3 / 3.5 (ROG Ally Radeon 780M), and RDNA 4 GPUs.
  - Includes built-in one-click download, Authenticode signature verification, and automated FSR 3.1 fallback.
- **NVIDIA DLSS & Model Version Selector (with DLSS 5 & Streamline Releases):**
  - References and integrates official [NVIDIA Streamline releases](https://github.com/NVIDIA-RTX/Streamline/releases) (`v2.14.16`) for DLSS Super Resolution and DLSS Frame Generation (`sl.interposer.dll`, `sl.dlss.dll`, `sl.dlss_g.dll`, `nvngx_dlssg.dll`).
  - Dynamic **DLSS Version** selector under **Emulation > Configure > Graphics > NVIDIA DLSS**.
  - Integrated support for the **DLSS 5** neural model architecture based on [Merserk/dlss5-visual-enhancer](https://github.com/Merserk/dlss5-visual-enhancer), selectable alongside **DLSS 4 (Preset K)**, **DLSS 3.10**, and **Auto (NVIDIA App OTA)** with Authenticode verification.

---

### 🚀 Multi-Vendor Frame Generation & Multipliers (2x, 3x, 4x)
- **Multi-Vendor Frame Generation Backends:**
  - **AMD FSR 3.1 / Redstone Frame Generation:** Built-in high-performance compute optical-flow bilateral frame synthesis, running universally across all GPU vendors without proprietary hardware locks.
  - **NVIDIA DLSS 3 Frame Generation (DLSS-G):** Streamline-driven DLSS-G (`nvngx_dlssg.dll`) with optical flow acceleration.
  - **Intel XeSS Frame Generation (XeSS-FG):** Intel's cutting-edge frame generator (`libxess_fg.dll` in `user/xess/`).
  - **Lossless Scaling (LSFG):** Seamless frame generation for low-overhead interpolation.
- **Frame Generation Multipliers (`2x`, `3x`, `4x`):**
  - Fully exposed frame multiplier controls:
    - **2x:** Generates 1 intermediate frame per rendered frame (e.g., 30 -> 60 FPS, 60 -> 120 FPS).
    - **3x:** Generates 2 intermediate frames per rendered frame (e.g., 30 -> 90 FPS).
    - **4x:** Generates 3 intermediate frames per rendered frame (e.g., 30 -> 120 FPS, 60 -> 240 FPS).
  - Configurable in **Emulation > Configure > Graphics** and accessible instantly on-the-fly via the **Dekubi Quick Overlay Menu**!

---

### 📊 Performance Overlay with Frame Times (Dekubi Green)
- **Real-Time GPU Submission Timestamps:** Live Vulkan GPU render execution time in milliseconds (`GPU ms`) and utilization (`% busy`) calculated via GPU queue timestamp pairs.
- **Process CPU & Bottleneck Identification:** Shows Dekubi's CPU utilization (`CPU %`) alongside an automated real-time bottleneck detector (`Limited by GPU`, `Front`, `Back`, `Render`, `Game`, or `Sync`; or `Capped` when hitting the target cap), lighting up in **Dekubi Green (`#2ecc40`)**.
- **Per-Thread Execution Breakdown:** Dedicated millisecond execution monitors for `Front` (command buffer submission), `Back` (worker translation), `Game` (CPU core), and `Render` (presentation).
- **Multi-Vendor FG Badging:** Displays active frame generator badges (`FSR 3.1`, `LSFG`, `DLSS-G`, `XeSS-FG`) and active multiplier directly beside the FPS readout.
- **View Menu Integration:** Easily toggle rows on/off via **View > Show Overlay Frame Times**.

---

### 🛡️ Single-Instance Protection & Process Safety
- **Single-Instance Guard:** Prevents launching duplicate emulator processes when opening games, ROM shortcuts, or clicking desktop shortcuts while Dekubi is already active.
- **Automatic Window Focus:** When a duplicate instance is triggered, Dekubi automatically locates the active window, restores it if minimized, brings it to the foreground with focus, and cleanly terminates the secondary process without interrupting your active game.
- **Resource Collision Prevention:** Protects against dual-process RAM/VRAM exhaustion (vital on 16 GB unified memory devices like the ROG Ally / Ally X), audio backend lockouts, and Vulkan presentation swapchain collisions.

---

### 🎨 UI & Settings Enhancements
- **Window Filter "Auto" Mode (Default):** Dynamically toggles between **Area** downsampling (when picture resolution exceeds window/screen dimensions by >5%, eliminating moiré and shimmering artifacts on 1080p handheld screens) and **ScaleForce** (for smaller images). Includes dynamic pros/cons and screen-matching advice directly inside Graphics Configuration.
- **Curated 360p–8K Resolution List:** Clean resolution dropdown displaying a single clean resolution label when docked and handheld match, filtered to standard resolutions from 360p up to 8K.
- **Tools > Switches Live Editor (`Tools > Switches...`):** Dedicated GUI window to inspect, edit, and categorize active switches (`nx2_switches.txt`) by source (User file, Environment, Built-in) with live syntax highlighting and one-click Save & Restart.
- **Framerate Cap Warning:** High refresh rate options now include an explicit warning: `Unlimited (can crash with VSync off)`.
- **Full Per-Game Settings Retained:** All per-game overrides, `custom/<title_id>.ini` configs, and individual "Use global" checkboxes remain completely intact.

---

### 🎮 Built-in Engine Patches & Game Mods Suite
- **Engine-Level Built-in Patches:**
  - **Dynamic FPS & High Refresh:** Dynamic tick rate scaling up to 120+ FPS.
  - **Texture Pool x2:** Doubles texture allocator limits, preventing memory crashes in high-density areas.
  - **Water Ripples above 60 FPS:** Preserves fluid ripple simulations at uncapped frame rates.
  - **Occlusion Off & Poll Backoff:** Reduces CPU stalls in dense geometry scenes.
  - **In-Game Mod Menu:** Quick-access toggle menu with hotkey controls.
- **User Game Mods Pre-Packaged:**
  - **The Legend of Zelda: BotW (`01007EF00011E000`):** 60 FPS mod, Dynamic Resolution Disable, and Improved Level of Detail (LOD).
  - **Pokémon Scarlet (`0100A3D008C5C000`):** Scarlet 60 FPS (v1.0.1).
  - **Pokémon Legends: Arceus (`0100F43008C44000`):** 60 FPS patch.
  - **Zero-Overhead LayeredFS Support (`040074A01BF12000`):** Direct directory junction linking for instant loading without disk duplication.
- **Precompiled Shader Recipes:**
  - Pre-packaged shader cache recipes under `recipes/` (e.g., `OoT v131072 (cache v54).nx2recipe`).

---

### 🕹️ Device & Memory Optimizations (ROG Ally & 4 GB VRAM / 8 GB RAM PCs)
- **8 GB PC Virtual Memory Guidance:** Checks available commit space at startup on 8 GB systems and suggests optimal pagefile sizing to prevent out-of-memory closures.
- **Lazy Guest Memory Commit (`EDEN_NX2_SEC_RESERVE=1`):** Commits guest memory pages on demand on ROG Ally and entry-tier PCs, saving ~5.5 GiB of host commit charge.
- **Idle Zero-Page Trimming (`EDEN_NX2_ZERO_TRIM=1`):** Periodically reclaims zero-filled guest memory pages during idle moments, freeing up 5.5–6.5 GiB physical RAM without micro-stutters.
- **Pot Room & Shadow Depth VRAM Reclaim:** Relocates and auto-retires covered shadow depth targets after 4 frames in Castle Town guard house / pot room, cutting VRAM usage by ~3.2 GiB (10.5 GB -> 7.3 GB).
- **Relocation Memory:** Persists render target relocation guards across sessions in `cache/relocation_lost.txt`.
- **ROG Ally & 8-Core CPU Spin Optimization:** Tuned worker thread spin locks from 80 µs down to 20 µs to eliminate CPU core starvation on 8C/16T Zen 4 handhelds.
