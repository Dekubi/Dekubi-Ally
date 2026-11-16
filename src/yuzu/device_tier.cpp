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
} // namespace

void Apply() {
    constexpr std::uint64_t GiB = 1024ull * 1024ull * 1024ull;
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    const std::uint64_t ram = mem.ullTotalPhys;
    const std::uint64_t vram = DedicatedVram();
    const std::string model = ProductName();
    const bool ally = Contains(model, "ROG Ally") || Contains(model, "RC71") || Contains(model, "RC72");

    const char* tier = "high";
    const char* tc = "2560";
    const char* evict = "1500";
    if (ally) {
        tier = "ally"; tc = "1536"; evict = "1000";
    } else if (ram <= 9 * GiB || (vram != 0 && vram <= 4 * GiB + GiB / 2)) {
        tier = "low"; tc = "1024"; evict = "600";
    } else if (vram != 0 && vram <= 8 * GiB + GiB / 2) {
        tier = "mid"; tc = "1536"; evict = "1000";
    }
    SetDefault("EDEN_NX2_TC_CEILING_MIB", tc);
    SetDefault("EDEN_NX2_PIPELINE_EVICT_ABOVE", evict);
    const std::string tier_name = tier;
    if (tier_name == "low") {
        SetDefault("EDEN_NX2_WARM_THREADS", "2");
    } else if (tier_name == "ally" || tier_name == "mid") {
        SetDefault("EDEN_NX2_WARM_THREADS", "4");
    }
    SetEnvironmentVariableA("DEKUBI_DEVICE_TIER", tier);
}
} // namespace DeviceTier
#else
namespace DeviceTier {
void Apply() {}
} // namespace DeviceTier
#endif