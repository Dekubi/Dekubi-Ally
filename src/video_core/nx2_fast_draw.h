// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <span>
#include <utility>
#include <vector>

#include "common/common_types.h"
#include "video_core/nx2_debug_control.h"

namespace Tegra {
class MemoryManager;
}

/// A draw that follows a draw of the same pipeline with only light state changes in between takes
/// the delta path. It keeps the previous draw's pipeline, image views, samplers and texture buffer
/// bindings, and resolves and binds every buffer (index, vertex, uniform, storage, texture) again
/// as a full resolve does, so guest memory written without a notification is streamed as in a
/// full resolve.
/// Light changes are draw parameters, index buffer and vertex stream registers, constant buffer
/// binds and inline constant buffer data (collected in g_delta: the rasterizer resolves in full
/// when they reach the pipeline's texture handles at their GPU addresses or through another
/// mapping of their device memory). Anything else that could change what a resolve produces bumps
/// a serial: a 3D register that changes, any 3D method with side effects (uploads, reports,
/// clears, barriers, ...), constant buffer data reaching the texture or sampler headers (the same
/// way), any other engine, deferred calls on the split's back (except the ones marked
/// GpuSplit::FastDrawNeutral), GPU::TickWork requests, texture cache writes, unmaps and
/// deletions, shader removals, fence releases, HLE macros and frames. The rasterizer takes the
/// delta path only while the serial, the GPU memory map, the command buffer, the render pass and
/// the bound pipeline are the ones the previous draw left.
/// EDEN_NX2_FAST_DRAW_VERIFY=1 resolves those draws in full and counts where the pipeline, views,
/// samplers or kept buffer bindings differ.
namespace VideoCommon::Nx2FastDraw {

enum class Reason : u8 {
    Register,
    Method,
    ConstantData,
    InlineData,
    OtherEngine,
    Call,
    TickWork,
    Invalidation,
    Fence,
    IndirectReset,
    Frame,
    TextureHeaders,
    DeltaOverflow,
    Map,
    Count,
};

inline constexpr std::array<const char*, static_cast<size_t>(Reason::Count)> reason_names{
    "register",      "method",    "constant buffer data", "inline data",
    "other engine",  "deferred call", "tick work",        "invalidation",
    "fence release", "indirect reset", "frame",           "texture header data",
    "delta overflow", "memory map",
};

/// Who sent a deferred call (GpuSplit::CallTag), for the "deferred call" breaks.
enum class CallSite : u8 {
    Other,
    Run,           ///< GpuSplit::Run, untagged
    QueryFlush,    ///< Macro DMA read: pending reports written out (QueryDma::FlushPending)
    MacroDmaFlush, ///< Macro DMA read: buffer cache flush
    PullerOnBack,  ///< Puller semaphores, syncpoints and cache operations
    ReleaseFences, ///< Puller fence release before a wait
    Function,      ///< GpuSplit::DeferFunction (GPU thread run commands)
    FlushRegion,   ///< GPU thread flush command
    Invalidate,    ///< GPU thread invalidate command, list-start invalidation
    BindChannel,
    ListEnd,       ///< Command list end: FlushCommands, OnCommandListEnd
    ListCheck,     ///< EDEN_NX2_LIST_WAIT_SKIP's check of a skipped wait
    Count,
};

inline constexpr std::array<const char*, static_cast<size_t>(CallSite::Count)> call_site_names{
    "other",          "run",         "query flush",  "macro dma flush", "puller",
    "release fences", "function",    "flush region", "invalidate",      "bind channel",
    "list end",       "list check",
};

/// EDEN_NX2_EXP64 (live, default 1; port of zortos293/Lichii#1403's reasoning): occlusion query
/// work keeps reuse (counter resets, ZPass reports, render enable, report flushes for macro DMA
/// reads, the list-wait check), and a full delta widens its last range instead of breaking.
/// 0 restores the breaks.
[[nodiscard]] inline bool QueriesNeutral() {
    return VideoCore::Nx2Debug::Experiment(64) != 0;
}

/// EDEN_NX2_EXP65 (live, default 1; zortos293/Lichii#1519): pipelines with handles computed at
/// draw time take delta draws, handle checks compare only the words handles are read from, a full
/// delta widens a range, and with dynamic vertex input attribute formats are draw parameters.
/// 0 restores the #858 behaviour.
[[nodiscard]] inline bool HandleDeltas() {
    return VideoCore::Nx2Debug::Experiment(65) != 0;
}

/// Whether pipelines take their vertex input as dynamic state (VK_EXT_vertex_input_dynamic_state):
/// then the vertex attribute formats are draw parameters, as only each attribute's type class is
/// in the pipeline key. Set by the pipeline cache before the GPU threads run, read on the back.
inline std::atomic<bool> g_dynamic_vertex_input{};

/// Whether pipelines take vertex stream strides as dynamic state (extended dynamic state and
/// dynamic vertex input): a stride change then leaves the pipeline key alone. Set with
/// g_dynamic_vertex_input.
inline std::atomic<bool> g_dynamic_strides{};

/// EDEN_NX2_EXP66 (live, default 1; zortos293/Lichii#974/#1216): a vertex stream's stride and
/// enable word (with dynamic strides) and the macro engine's memory read addresses are draw
/// parameters. 0 restores the breaks.
[[nodiscard]] inline bool StrideDeltas() {
    return VideoCore::Nx2Debug::Experiment(66) != 0;
}

/// Any thread: whether fast draws run. Dekubi: the live switch EDEN_NX2_EXP15 (default 1; 0 =
/// off, for A/B tests). The rasterizer latches it per draw (on the thread that draws) and starts
/// with a full resolve when it turns on.
[[nodiscard]] inline bool Enabled() {
    return VideoCore::Nx2Debug::Experiment(15) != 0;
}

/// Tests only.
inline void SetEnabledForTest(bool on) {
    VideoCore::Nx2Debug::LoadExperiments();
    VideoCore::Nx2Debug::g_experiments[15].store(on ? 1 : 0, std::memory_order_relaxed);
}

/// EDEN_NX2_FAST_DRAW_VERIFY=1, read once.
[[nodiscard]] inline bool VerifyEnabled() {
    static const bool verify = [] {
        const char* const value = std::getenv("EDEN_NX2_FAST_DRAW_VERIFY");
        return value != nullptr && value[0] == '1';
    }();
    return verify;
}

inline std::atomic<u64> g_serial{1};
/// The last break's reason and, for Register and Method, its 3D method (diagnostics).
inline std::atomic<u32> g_last_break{};

inline void Break(Reason reason, u32 method = 0) {
    if (Enabled()) {
        g_serial.fetch_add(1, std::memory_order_relaxed);
        g_last_break.store(static_cast<u32>(reason) | (method << 8), std::memory_order_relaxed);
    }
}

inline u64 Serial() {
    return g_serial.load(std::memory_order_relaxed);
}

/// The device memory a GPU range maps to. A write through another GPU address that maps the
/// same device memory changes the range too.
class DeviceSpans {
public:
    /// Translates [address, address + size) again unless that range of memory was translated last
    /// and memory's mappings haven't changed since.
    void Update(const Tegra::MemoryManager& memory, GPUVAddr address, u64 size);

    /// Whether [begin, end) of device memory overlaps the range's.
    [[nodiscard]] bool Intersects(DAddr begin, DAddr end) const;

    /// The GPU address the range starts at.
    [[nodiscard]] GPUVAddr Address() const noexcept {
        return address;
    }

    /// Whether the range maps to one run of device memory in GPU order, starting at LinearBase.
    [[nodiscard]] bool IsLinear() const noexcept {
        return linear;
    }
    [[nodiscard]] DAddr LinearBase() const noexcept {
        return linear_base;
    }

private:
    const Tegra::MemoryManager* memory{};
    GPUVAddr address{};
    u64 size{};
    u64 map_serial{};
    bool linear{};
    DAddr linear_base{};
    std::vector<std::pair<DAddr, DAddr>> spans; ///< Sorted, disjoint [begin, end) pairs
};

/// Light changes since the last draw that can reach texture handles. Written and read on the
/// thread that runs the 3D engine and the rasterizer (the split's back).
struct Delta {
    static constexpr size_t MaxWrites = 16;

    std::array<u32, 5> binds{}; ///< Constant buffer slots bound per stage
    /// Constant buffer data written, by GPU address (unmapped addresses included)...
    std::array<GPUVAddr, MaxWrites> write_begin{};
    std::array<GPUVAddr, MaxWrites> write_end{};
    u32 num_writes{};
    /// ...and by the device address it landed on.
    std::array<DAddr, MaxWrites> device_begin{};
    std::array<DAddr, MaxWrites> device_end{};
    u32 num_device_writes{};

    [[nodiscard]] bool Empty() const {
        return num_writes == 0 && (binds[0] | binds[1] | binds[2] | binds[3] | binds[4]) == 0;
    }

    /// Whether data written reaches [address, address + size), whose device memory is spans: at
    /// one of its GPU addresses, or through any GPU address that maps its device memory.
    [[nodiscard]] bool Reaches(GPUVAddr address, u64 size, const DeviceSpans& spans) const {
        for (u32 write = 0; write < num_writes; ++write) {
            if (write_begin[write] < address + size && address < write_end[write]) {
                return true;
            }
        }
        for (u32 write = 0; write < num_device_writes; ++write) {
            if (spans.Intersects(device_begin[write], device_end[write])) {
                return true;
            }
        }
        return false;
    }

    /// Reaches for only the bytes [base + first, base + second) of each of words, all inside the
    /// range spans was updated with. Device writes are compared word by word when that range maps
    /// linearly, else with the whole range.
    [[nodiscard]] bool ReachesWords(GPUVAddr base, std::span<const std::pair<u32, u32>> words,
                                    const DeviceSpans& spans) const {
        for (u32 write = 0; write < num_writes; ++write) {
            for (const auto& [first, second] : words) {
                if (write_begin[write] < base + second && base + first < write_end[write]) {
                    return true;
                }
            }
        }
        for (u32 write = 0; write < num_device_writes; ++write) {
            if (!spans.Intersects(device_begin[write], device_end[write])) {
                continue;
            }
            if (!spans.IsLinear()) {
                return true;
            }
            for (const auto& [first, second] : words) {
                const DAddr begin = spans.LinearBase() + (base + first - spans.Address());
                const DAddr end = begin + (second - first);
                if (device_begin[write] < end && begin < device_end[write]) {
                    return true;
                }
            }
        }
        return false;
    }
};

inline Delta g_delta;

/// Moves g_delta into `out` and clears it, copying only the ranges in use (Delta is ~0.5 KiB and
/// taken for every draw).
inline void TakeDelta(Delta& out) {
    Delta& delta = g_delta;
    out.binds = delta.binds;
    out.num_writes = delta.num_writes;
    std::copy_n(delta.write_begin.begin(), delta.num_writes, out.write_begin.begin());
    std::copy_n(delta.write_end.begin(), delta.num_writes, out.write_end.begin());
    out.num_device_writes = delta.num_device_writes;
    std::copy_n(delta.device_begin.begin(), delta.num_device_writes, out.device_begin.begin());
    std::copy_n(delta.device_end.begin(), delta.num_device_writes, out.device_end.begin());
    delta.binds = {};
    delta.num_writes = 0;
    delta.num_device_writes = 0;
}

/// Adds [begin, end) to a delta's range list, or extends its last range when that ends at begin.
/// With HandleDeltas (#1519) it extends a range it overlaps, and a full list widens the range that
/// grows least; otherwise a full list widens its last range with QueriesNeutral (#1403). Wider only
/// makes Reaches more conservative. False when the write was not recorded (a full list then).
inline bool AppendRange(std::array<u64, Delta::MaxWrites>& begins,
                        std::array<u64, Delta::MaxWrites>& ends, u32& count, u64 begin, u64 end) {
    if (count != 0 && ends[count - 1] == begin) {
        ends[count - 1] = end;
        return true;
    }
    const bool widen = HandleDeltas();
    if (widen) {
        for (u32 i = 0; i < count; ++i) {
            if (begin <= ends[i] && begins[i] <= end) {
                begins[i] = (std::min)(begins[i], begin);
                ends[i] = (std::max)(ends[i], end);
                return true;
            }
        }
    }
    if (count < Delta::MaxWrites) {
        begins[count] = begin;
        ends[count] = end;
        ++count;
        return true;
    }
    u32 best = count - 1;
    if (widen) {
        u64 best_growth = ~u64{0};
        for (u32 i = 0; i < count; ++i) {
            const u64 growth = ((std::max)(ends[i], end) - (std::min)(begins[i], begin)) -
                               (ends[i] - begins[i]);
            if (growth < best_growth) {
                best = i;
                best_growth = growth;
            }
        }
    } else if (!QueriesNeutral()) {
        return false;
    }
    begins[best] = (std::min)(begins[best], begin);
    ends[best] = (std::max)(ends[best], end);
    return true;
}

inline void NoteBind(size_t stage, u32 slot) {
    if (Enabled()) {
        g_delta.binds[stage] |= 1U << slot;
    }
}

/// Inline constant buffer data written to GPU addresses [begin, end).
inline void NoteConstantData(GPUVAddr begin, GPUVAddr end) {
    if (!Enabled()) {
        return;
    }
    Delta& delta = g_delta;
    if (!AppendRange(delta.write_begin, delta.write_end, delta.num_writes, begin, end)) {
        Break(Reason::DeltaOverflow);
    }
}

/// The device addresses [begin, end) that inline constant buffer data landed on.
inline void NoteDeviceData(DAddr begin, DAddr end) {
    if (!Enabled()) {
        return;
    }
    Delta& delta = g_delta;
    if (!AppendRange(delta.device_begin, delta.device_end, delta.num_device_writes, begin, end)) {
        Break(Reason::DeltaOverflow);
    }
}

} // namespace VideoCommon::Nx2FastDraw
