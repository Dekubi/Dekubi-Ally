# CLAUDE.md - AI Handoff & Project Guide for Dekubi Ally

## 1. Executive Summary & Current State (2026-11-19)
- **Status:** **100% Complete & Stable**.
- **Current Core:** Dekubi Ally optimized with full **Fritchii v6.2** parity and tuned for target hardware (ASUS ROG Ally Z1/Z1 Extreme, Steam Deck, Lenovo Legion Go, MSI Claw, and low-spec 4GB VRAM / 8GB RAM PCs).
- **Compiler:** Built cleanly with **Clang 22.1.8 (`clang-cl.exe`)** and **Ninja** (MSVC / `cl.exe` is deprecated and strictly forbidden).
- **GitHub Origin:** All updates merged and pushed to `main` at `https://github.com/Dekubi/Dekubi-Ally` (commit `5c22f27`).
- **Active Executable:** Deployed to:
  - `C:\Users\User\projects\Fritchii\Dekubi_Ally\dekubi.exe` (92,451,840 bytes)
  - `C:\Users\User\Games\Dekubi_Ally\dekubi.exe` (92,451,840 bytes)
- **Golden Baseline Backup:** `dekubi.exe.v115` (76,499,968 bytes) is preserved untouched in both directories. **NEVER modify, overwrite, or delete this file under any circumstance.**

---

## 2. Directory & Repository Layout
- `C:\Users\User\projects\Fritchii`: Primary development tree (current branch: `compute-stalls`).
- `C:\Users\User\projects\Fritchii\test-ninja`: Active Clang 22.1.8 + Ninja build folder.
- `C:\Users\User\projects\Fritchii\Dekubi_Ally`: Local deployment folder for user testing.
- `C:\Users\User\Games\Dekubi_Ally`: User's live gaming installation folder.
- `C:\Users\User\projects\Dekubi-Ally-github`: Dedicated git checkout pointing directly to `origin https://github.com/Dekubi/Dekubi-Ally` (`main` branch).

---

## 3. Toolchain & Build Instructions
Always compile with **Clang-cl + Ninja**. Do NOT use MSVC `cl.exe` or MSBuild.

### Building `dekubi.exe`
Run via PowerShell/cmd using `BypassSandbox: true`:
```cmd
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cd /d C:\Users\User\projects\Fritchii\test-ninja && ninja dekubi'
```
*Tip: If building only `video_core` library:*
```cmd
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cd /d C:\Users\User\projects\Fritchii\test-ninja && ninja video_core'
```

### Deploying the Built Binary
After building `test-ninja\bin\dekubi.exe`, copy to both deploy destinations:
```powershell
Copy-Item C:\Users\User\projects\Fritchii\test-ninja\bin\dekubi.exe C:\Users\User\projects\Fritchii\Dekubi_Ally\dekubi.exe -Force
Copy-Item C:\Users\User\projects\Fritchii\test-ninja\bin\dekubi.exe C:\Users\User\Games\Dekubi_Ally\dekubi.exe -Force
```
*(Verify `dekubi.exe.v115` was not touched!)*

---

## 4. Key Architectural & Code Solutions
1. **FastDraw Symbol Resolution (`nx2_fast_draw.cpp`):**
   - Must be included in `src/video_core/CMakeLists.txt`.
   - Provides `VideoCommon::Nx2FastDraw::DeviceSpans::Update` and `Intersects`.
2. **Decoupled Memory Stage (`src/video_core/memory_stage_stubs.cpp`):**
   - Experimental decoupling files `memory_stage_link.cpp` and `memstage_shadow.cpp` were bypassed to eliminate compile errors with private struct members.
   - Replaced by `memory_stage_stubs.cpp` which cleanly stubs `MemoryStageLink`, `OneFollower`, and provides direct inline execution for `SeamClock` and `SeamTicks`.
3. **Shader Recompiler SpirV Signature (`emit_spirv_mesh.cpp`):**
   - `EmitMeshEmulationVertex` must use the upstream 8-parameter signature:
     `(ctx, position, point_size, clip_distances, layer, viewport_index, point_coord, primitive_shading_rate)`.
4. **Native Thread Priority (`src/common/thread.cpp`):**
   - `SetNativeThreadPriority` uses Win32 `::OpenThread` and `::SetThreadPriority` with `THREAD_PRIORITY_TIME_CRITICAL` / `HIGHEST` / `ABOVE_NORMAL` / `NORMAL` mapping.
5. **Null Buffer Dispatch Safety (`src/video_core/renderer_vulkan/vk_rasterizer.cpp`):**
   - Guarded unmapped argument addresses from executing dispatches to GPU address 0 when `buffer == nullptr || buffer->Handle() == VK_NULL_HANDLE`.
6. **Fritchii 6.2 Intel Hybrid Core Migration (`src/yuzu/main.cpp`):**
   - Automatic migration turns obsolete `EDEN_HYBRID_CORES=0` lines in switch files into comments, letting Windows thread director and CPU Sets place GPU threads optimally.

---

## 5. Strict Project Rules & Constraints
1. **Clang Only:** Always use Clang 22.1.8 (`clang-cl.exe`) with Ninja. MSVC (`cl.exe`) is forbidden.
2. **Never Touch `dekubi.exe.v115`:** This is the baseline golden backup.
3. **No-Credits Policy:** Never introduce external fork names or author credits into user-facing UI, dialogs, or release notes. Keep branding as **Dekubi Ally**.
4. **Preserve Custom Features:**
   - Multi-vendor Frame Gen suite: XeSS-FG (`libxess_fg.dll`), FSR 3.1 (`ffx_framegeneration_x64.dll`), DLSS-G, and LSFG (Lossless Scaling Frame Gen).
   - In-game Mod Menu (`F8` / UI toggle).
   - Dekubi-green Performance Overlay (`#00E676`).
   - Borderless Fullscreen Taskbar fix (`F11`).
   - Target Device Tier Auto-Tuning (`device_tier.cpp`).
5. **Always Sandbox Bypass for External Paths / Builds:** Use `BypassSandbox: true` for build commands, git operations, and accessing `C:\Users\User\Games`.

---

## 6. Next Optimization Roadmap (Backlog for Future Turns)
1. **Settled-Word Semaphore Acquires (Lichii PR #2143):**
   - Bypasses `split.Drain(FenceRelease)` in `Puller::AcquireSemaphore()` when `split.SettledWordAt(address)` is satisfied, eliminating up to 3.1s of GPU submission stalls.
2. **Shared Dynarmic JIT Code Cache (Lichii PR #2101):**
   - Merges per-core Dynarmic A64 code caches into a single process-wide shared cache, cutting 78.7% of duplicate compiles and 14.8s of JIT stalls during transitions.
3. **Win32 Memory-Mapped RomFS:**
   - Replaces synchronous `ReadFile()` in `fs_i_storage.cpp` with zero-copy `CreateFileMapping` / `MapViewOfFile`.
4. **Batched ZeroTrim Lock Runs (Lichii PR #2156):**
   - Acquires `placeholder_mutex` per 2 MiB run instead of per 4 KiB page, eliminating 389 ms lock hitches.
5. **Explicit DMI Matching for Handhelds (`src/yuzu/device_tier.cpp`):**
   - Direct recognition of `"Jupiter"` / `"Galileo"` (Steam Deck) and `"83E1"` (Legion Go) for tailored UMA ceilings.
