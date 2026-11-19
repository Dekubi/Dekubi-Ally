#pragma once

#include <string>
#include <utility>
#include <vector>

namespace DeviceTier {
// NX2 tuning values for the detected RAM, VRAM and device model. They replace the built-in
// switches; nx2_switches.txt and the real environment still win over them.
std::vector<std::pair<std::string, std::string>> Defaults();

// Publishes the detected tier as DEKUBI_DEVICE_TIER (for the log export).
void Apply();
}
