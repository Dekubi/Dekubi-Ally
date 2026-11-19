#include "yuzu/device_tier.h"

#ifdef _WIN32
#include <windows.h>
#include <dxgi.h>
#include <cstdint>
#include <string>

namespace DeviceTier {
namespace {
void SetDefault(const char* key, const char* value) {
    if (GetEnvironmentVariableA(key, nullptr, 0) == 0) {
        SetEnvironmentVariableA(key, value);
        _putenv_s(key, value);
    }
}

std::string ProductName() {
    char buf[256]{};
    DWORD size = sizeof(buf);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", "SystemProductName",
                     RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS) {
        return {};
    }
    return buf;
}

std::uint64_t DedicatedVram() {
    std::uint64_t best = 0;
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
        return 0;
    }
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
            desc.DedicatedVideoMemory > best) {
            best = desc.DedicatedVideoMemory;
        }
        adapter->Release();
    }
    factory->Release();
    return best;
}

bool Contains(const std::string& s, const char* needle) {
    return s.find(needle) != std::string::npos;
}

const char* DetectTier() {
    constexpr std::uint64_t GiB = 1024ull * 1024ull * 1024ull;
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    const std::uint64_t ram = mem.ullTotalPhys;
    const std::uint64_t vram = DedicatedVram();
    const std::string model = ProductName();
    const bool ally = Contains(model, "ROG Ally") || Contains(model, "RC71") || Contains(model, "RC72");
    const bool deck = Contains(model, "Jupiter") || Contains(model, "Galileo") || Contains(model, "Steam Deck");
    const bool legion = Contains(model, "Legion Go") || Contains(model, "83E1");
    const bool other_handheld = Contains(model, "AYANEO") || Contains(model, "AyaNeo") ||
                                Contains(model, "G161") || Contains(model, "GPD") ||
                                Contains(model, "Claw");
    if (deck) {
        return "deck";
    }
    if (ally || legion || other_handheld) {
        return "ally";
    }
    if (ram <= 9 * GiB || (vram != 0 && vram <= 4 * GiB + GiB / 2)) {
        return "low";
    }
    if (vram != 0 && vram <= 8 * GiB + GiB / 2) {
        return "mid";
    }
    return "high";
}

const std::string& Tier() {
    static const std::string tier = DetectTier();
    return tier;
}
} // namespace

std::vector<std::pair<std::string, std::string>> Defaults() {
    // No pipeline eviction limit here: the core's RAM-based default (EvictAbovePipelines, at least
    // 2,000) applies. Limits of 600-1,500 made areas rebuild their evicted pipelines on the GPU
    // thread when revisited, costing frame rate on every tier.
    // Texture cache ceilings (the built-in 7,680 MiB suits large cards): the ROG Ally shares its
    // 16 GB between the system and the GPU, and v1.1.2 filled it on the way to Hyrule Field (0 GB
    // VRAM and under 1 GB RAM free, then a freeze and a crash). The texture cache ignores
    // ceilings under 2,048 MiB.
    const std::string& tier = Tier();
    if (tier == "deck") {
        return {
            {"EDEN_NX2_TC_CEILING_MIB", "2560"},
            {"EDEN_NX2_WARM_THREADS", "2"},
            // Commits guest pages on demand, saving ~5.5 GiB host commit charge.
            {"EDEN_NX2_SEC_RESERVE", "1"},
            // Sheds zero-filled guest heap pages from working set, saving ~5.5-6.5 GiB physical RAM.
            {"EDEN_NX2_ZERO_TRIM", "1"},
            // Pauses trimming during bulk mapping and backs off when idle, eliminating hitches.
            {"EDEN_NX2_ZERO_TRIM_IDLE", "1"},
            // Handheld battery & CPU scheduling tuning
            {"FRITCHII_WORKER_SPIN", "20"},
            {"EDEN_NX2_WORKER_SPIN", "20"},
            {"FRITCHII_ONEF_AMD", "0"},
            {"FRITCHII_SMALL_CARD", "2"},
            {"FRITCHII_TC_THRASH_GUARD", "1"},
            {"FRITCHII_HOST_ZSTD", "3"},
            {"FRITCHII_OCCL_FLAG", "3"},
            {"FRITCHII_COVERED_RETIRE", "4"},
            {"EDEN_NX2_HANDHELD_IGPU_LIMIT", "1300"},
            {"EDEN_NX2_OOT_HIGH_REFRESH", "1"},
            {"EDEN_NX2_OOT_HIGH_REFRESH_UNLIMITED", "1"},
        };
    }
    if (tier == "ally") {
        return {
            {"EDEN_NX2_TC_CEILING_MIB", "3072"},
            {"EDEN_NX2_WARM_THREADS", "4"},
            // Commits guest pages on demand, saving ~5.5 GiB host commit charge.
            {"EDEN_NX2_SEC_RESERVE", "1"},
            // Sheds zero-filled guest heap pages from working set, saving ~5.5-6.5 GiB physical RAM.
            {"EDEN_NX2_ZERO_TRIM", "1"},
            // Pauses trimming during bulk mapping and backs off when idle, eliminating hitches.
            {"EDEN_NX2_ZERO_TRIM_IDLE", "1"},
            // Handheld battery & CPU scheduling tuning
            {"FRITCHII_WORKER_SPIN", "20"},
            {"EDEN_NX2_WORKER_SPIN", "20"},
            {"FRITCHII_ONEF_AMD", "0"},
            {"FRITCHII_SMALL_CARD", "2"},
            {"FRITCHII_TC_THRASH_GUARD", "1"},
            {"FRITCHII_HOST_ZSTD", "3"},
            {"FRITCHII_OCCL_FLAG", "3"},
            {"FRITCHII_COVERED_RETIRE", "4"},
            {"EDEN_NX2_HANDHELD_IGPU_LIMIT", "1300"},
            {"EDEN_NX2_OOT_HIGH_REFRESH", "1"},
            {"EDEN_NX2_OOT_HIGH_REFRESH_UNLIMITED", "1"},
        };
    }
    if (tier == "low") {
        return {
            {"EDEN_NX2_TC_CEILING_MIB", "2560"},
            {"EDEN_NX2_WARM_THREADS", "2"},
            // Commits guest pages on demand, saving ~5.5 GiB host commit charge.
            {"EDEN_NX2_SEC_RESERVE", "1"},
            // Sheds zero-filled guest heap pages from working set, saving ~5.5-6.5 GiB physical RAM.
            {"EDEN_NX2_ZERO_TRIM", "1"},
            // Pauses trimming during bulk mapping and backs off when idle, eliminating hitches.
            {"EDEN_NX2_ZERO_TRIM_IDLE", "1"},
            {"FRITCHII_WORKER_SPIN", "20"},
            {"EDEN_NX2_WORKER_SPIN", "20"},
            {"FRITCHII_SMALL_CARD", "2"},
            {"FRITCHII_TC_THRASH_GUARD", "1"},
            {"FRITCHII_HOST_ZSTD", "3"},
            {"FRITCHII_OCCL_FLAG", "3"},
            {"FRITCHII_COVERED_RETIRE", "4"},
            {"EDEN_NX2_HANDHELD_IGPU_LIMIT", "1300"},
            {"EDEN_NX2_OOT_HIGH_REFRESH", "1"},
            {"EDEN_NX2_OOT_HIGH_REFRESH_UNLIMITED", "1"},
        };
    }
    if (tier == "mid") {
        return {
            {"EDEN_NX2_OOT_HIGH_REFRESH", "1"},
            {"EDEN_NX2_OOT_HIGH_REFRESH_UNLIMITED", "1"},
            {"FRITCHII_REPORT_WAIT", "1"},
            {"EDEN_NX2_EXP30", "1"},
            {"EDEN_NX2_WARM_THREADS", "4"},
            {"EDEN_NX2_EXP47", "1"},
            {"FRITCHII_GPU_PRIORITY", "2"},
            {"FRITCHII_READ_SIDE_COPY", "3"},
        };
    }
    return {
        {"EDEN_NX2_OOT_HIGH_REFRESH", "1"},
        {"EDEN_NX2_OOT_HIGH_REFRESH_UNLIMITED", "1"},
        {"FRITCHII_REPORT_WAIT", "1"},
        {"EDEN_NX2_EXP30", "1"},
        {"EDEN_NX2_EXP47", "1"},
        {"FRITCHII_GPU_PRIORITY", "2"},
        {"FRITCHII_READ_SIDE_COPY", "3"},
    };
}

void Apply() {
    for (const auto& [key, value] : Defaults()) {
        SetDefault(key.c_str(), value.c_str());
    }
    SetEnvironmentVariableA("DEKUBI_DEVICE_TIER", Tier().c_str());
    _putenv_s("DEKUBI_DEVICE_TIER", Tier().c_str());

    // On handhelds and laptops, prevent Windows from putting emulator threads
    // into Power Throttling (EcoQoS) mode which drops clocks to 1.2 GHz on battery.
    PROCESS_POWER_THROTTLING_STATE power_throttling{};
    power_throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    power_throttling.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
    power_throttling.StateMask = 0;
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &power_throttling, sizeof(power_throttling));
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
}
} // namespace DeviceTier
#else
namespace DeviceTier {
std::vector<std::pair<std::string, std::string>> Defaults() {
    return {};
}
void Apply() {}
} // namespace DeviceTier
#endif