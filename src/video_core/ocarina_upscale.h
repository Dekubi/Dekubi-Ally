// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>

#include "common/settings_enums.h"

namespace VideoCommon {

/// OoT's picture this session, for the status bar: the upscaler that really runs and its sizes.
/// The rasterizer publishes it once when it picks OoT's upscaler; empty before that and outside OoT.
/// Backend: 1 the game's own, 2 FSR 3.1, 3 FSR 4, 4 DLSS, 5 XeSS. A 0 output means no upscaler (the resolution setting applies).
struct OcarinaPicture {
    std::uint32_t backend;
    std::uint32_t input_height;
    bool input_up_to; ///< The game's dynamic resolution (Max Resolution off): at most input_height
    std::uint32_t output_height;
};

inline std::atomic<std::uint64_t> g_ocarina_picture{0};

inline void PublishOcarinaPicture(std::uint32_t backend, std::uint32_t input_height, bool up_to,
                                  std::uint32_t output_height) {
    g_ocarina_picture.store((std::uint64_t{1} << 63) | (std::uint64_t{backend & 0xff} << 48) |
                                (std::uint64_t{up_to} << 47) |
                                (std::uint64_t{input_height & 0xffff} << 16) |
                                std::uint64_t{output_height & 0xffff},
                            std::memory_order_relaxed);
}

inline void ClearOcarinaPicture() {
    g_ocarina_picture.store(0, std::memory_order_relaxed);
}

[[nodiscard]] inline std::optional<OcarinaPicture> ReadOcarinaPicture() {
    const std::uint64_t value = g_ocarina_picture.load(std::memory_order_relaxed);
    if ((value >> 63) == 0) {
        return std::nullopt;
    }
    return OcarinaPicture{static_cast<std::uint32_t>((value >> 48) & 0xff),
                          static_cast<std::uint32_t>((value >> 16) & 0xffff),
                          ((value >> 47) & 1) != 0, static_cast<std::uint32_t>(value & 0xffff)};
}

enum class OcarinaUpscaleBackend { Native, Fsr, Dlss, Xess };

constexpr OcarinaUpscaleBackend SelectOcarinaUpscaler(bool fsr_enabled, bool dlss_enabled,
                                                    bool dlss_available,
                                                    std::string_view override) {
    if (override == "native") {
        return OcarinaUpscaleBackend::Native;
    }
    if (override == "fsr") {
        return OcarinaUpscaleBackend::Fsr;
    }
    if (override == "xess") {
        return OcarinaUpscaleBackend::Xess;
    }
    if ((dlss_enabled || override == "dlss") && dlss_available) {
        return OcarinaUpscaleBackend::Dlss;
    }
    return fsr_enabled ? OcarinaUpscaleBackend::Fsr : OcarinaUpscaleBackend::Native;
}

/// Whether the "OoT anti-aliasing" choice asks for AMD FSR (3.1, or 4 with FSR 3.1 as fallback).
constexpr bool OcarinaFsrChosen(Settings::OotAntiAliasing choice) {
    return choice == Settings::OotAntiAliasing::Fsr3 || choice == Settings::OotAntiAliasing::Fsr4;
}

/// The backend for the "OoT anti-aliasing" choice and the NVIDIA DLSS setting. An FSR choice wins
/// over DLSS; with Game or No sharpening, DLSS replaces the game's pass when it's on and available
/// (as before FSR came in).
constexpr OcarinaUpscaleBackend SelectOcarinaUpscaler(Settings::OotAntiAliasing choice,
                                                    bool dlss_enabled, bool dlss_available,
                                                    std::string_view override) {
    if (override == "native") {
        return OcarinaUpscaleBackend::Native;
    }
    if (override == "fsr") {
        return OcarinaUpscaleBackend::Fsr;
    }
    if (override == "xess") {
        return OcarinaUpscaleBackend::Xess;
    }
    if (override == "dlss" && dlss_available) {
        return OcarinaUpscaleBackend::Dlss;
    }
    if (choice == Settings::OotAntiAliasing::Xess) {
        return OcarinaUpscaleBackend::Xess;
    }
    const bool fsr = OcarinaFsrChosen(choice);
    return SelectOcarinaUpscaler(fsr, dlss_enabled && !fsr, dlss_available, override);
}

/// EDEN_NX2_DLSS_JITTER (boot, 0 = off): DLSS gets the camera jitter and the texture LOD bias
/// the FSR path uses (a player's request; DLSS is made for jittered input). 1 = the jitter as FSR
/// gets it, 2 = with its vertical sign flipped (Streamline's convention is unverified here).
inline int OcarinaDlssJitterMode() {
    static const int mode = [] {
        const char* value = std::getenv("EDEN_NX2_DLSS_JITTER");
        return value != nullptr ? std::atoi(value) : 0;
    }();
    return mode;
}

/// Texture LOD bias added to Ocarina's mipmapped samplers while FSR or XeSS upscales. AMD recommends
/// log2(render / output) - 1: -1.58 for docked 1440p -> 4K. Intel XeSS and DLSS get
/// log2(render / output): -0.58.
inline float OcarinaFsrLodBias(OcarinaUpscaleBackend backend) {
    if (backend == OcarinaUpscaleBackend::Fsr) {
        return -1.58f;
    }
    if (backend == OcarinaUpscaleBackend::Xess) {
        return -0.58f;
    }
    return backend == OcarinaUpscaleBackend::Dlss && OcarinaDlssJitterMode() != 0 ? -0.58f : 0.0f;
}

/// Whether a sampler with that bias keeps it for an image: only docked (handheld isn't measured),
/// not for images the GPU wrote (render targets would read a mip too sharp), and not once the
/// upscaler failed this session (the game's own pass runs again).
constexpr bool KeepOcarinaFsrLodBias(bool docked, bool upscaler_failed, bool gpu_written) {
    return docked && !upscaler_failed && !gpu_written;
}

} // namespace VideoCommon
