// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Copyright Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <compare>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "common/net/net.h"

namespace UpdateChecker {

enum class Failure {
    None,
    // GitHub refused the request because of rate limiting.
    RateLimited,
    // GitHub answered with an HTTP error.
    Http,
    // No usable answer arrived, for example when offline.
    Network,
};

struct CheckResult {
    std::optional<Common::Net::Release> update;
    // The package to download for `update` on Dekubi builds.
    std::optional<Common::Net::Asset> asset;
    Failure failure{Failure::None};
    int status{};
};

// Classify a failed release request by its HTTP status, or 0 when no response arrived.
Failure ClassifyFailure(int status);

CheckResult Check();
std::optional<Common::Net::Release> GetUpdate();
std::optional<Common::Net::Release> SelectUpdate(
    std::span<const Common::Net::Release> releases, std::string_view build_tag,
    std::string_view platform, Common::Net::LinuxPackage package = Common::Net::LinuxPackage::Any);

// Dekubi (formerly Frizti) releases are numbered N or N.M with one decimal: 4 < 4.1 < 4.9 < 5.
struct DekubiVersion {
    u32 major{};
    u32 minor{};
    u32 patch{}; ///< 5.6.1 (from v5.6 on: older builds read one decimal digit only)

    auto operator<=>(const DekubiVersion&) const = default;
    std::string ToString() const;
};

// "4", "3.5", "5.10" or "5.6.1", as in dist/frizti_version.txt.
std::optional<DekubiVersion> ParseDekubiVersion(std::string_view text);

// The build version "Dekubi v4" (or the older "Frizti v3.5"); nullopt for other builds.
std::optional<DekubiVersion> ParseBuildVersion(std::string_view build_version);

// The version of the running build, from Common::g_build_version.
std::optional<DekubiVersion> RunningVersion();

// "dekubi-v4", "dekubi-v4.1", or the older "frizti-v3".
std::optional<DekubiVersion> ParseDekubiTag(std::string_view tag);

// The Windows package, "Dekubi_Ally_v1.1.2_Windows_x64.zip". Releases with other names are never
// installed.
std::string DekubiAssetName(const DekubiVersion& version);

// The Linux AppImage, "Dekubi_Ally-v1.1.2-linux-x86_64.AppImage".
std::string DekubiAppImageName(const DekubiVersion& version);

// Where Dekubi releases are listed and downloaded from.
struct Source {
    // Scheme and authority of the release list, such as "https://api.github.com".
    std::string origin;
    std::string api_path;
    // Set by EDEN_UPDATE_URL: packages are then accepted from `origin` instead of GitHub.
    bool test{};
};

Source DefaultSource();

// Test-only release list from EDEN_UPDATE_URL, such as "http://127.0.0.1:8000/releases.json".
// Plain HTTP is accepted only for a loopback host.
std::optional<Source> ParseSourceOverride(std::string_view url);

// The Windows package of a Dekubi release, when its URL belongs to `source`.
std::optional<Common::Net::Asset> SelectDekubiAsset(const Common::Net::Release& release,
                                                      const Source& source);

// Put in a release's notes to keep the updater from ever offering it (a joke release, a test
// build): an HTML comment, so GitHub shows nothing of it on the release page.
inline constexpr std::string_view HiddenReleaseMarker = "<!-- dekubi:no-update -->";

// Whether the release's notes carry HiddenReleaseMarker.
[[nodiscard]] bool HiddenFromUpdater(const Common::Net::Release& release);

// The newest release that is newer than `current`, has a Windows package and isn't hidden from
// the updater.
std::optional<Common::Net::Release> SelectDekubiUpdate(
    std::span<const Common::Net::Release> releases, const DekubiVersion& current,
    const Source& source);

} // namespace UpdateChecker

