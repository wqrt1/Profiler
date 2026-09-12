#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <format>
#include <chrono>
#include <vector>
#include <string>
#include <cstddef>
#include <unordered_map>
#include <algorithm>

#include <buffer.h>
#include <sampler.h>
#include <cstdio>
#include <fstream>

std::string resolve_address(HANDLE hProcess, DWORD64 address, const profiler_options& options) {
    if (address == 0)
        return "<null>";

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};

    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);

    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;

    if(!SymFromAddr(hProcess, address, &displacement, symbol)) {
        if (options.verbose) {
            std::cerr << std::format("SymFromAddr failed for 0x{:X}: error={}\n", address, GetLastError());
        }
        return "<unkown>";
    }

    if (options.verbose) {
        std::cout << std::format(
            "address=0x{:X} symbol={} "
            "symbolBase=0x{:X} displacement=0x{:X}\n",
            address, symbol->Name,
            symbol->Address, displacement
        );
    }

    return std::string(symbol->Name);
}

auto resolve_all_samples(HANDLE hProcess, RingBuffer samples, const profiler_options& options) {
    std::vector<sampleSIM> resolved_samples{samples.get_sample_count()};

    static std::unordered_map<DWORD64, std::string> address_cache;

    for(Sample s : samples) {
        std::vector<std::string> callstack{};
        for(std::size_t i{}; i < s.frame_count; i++) {
            const auto address = s.addresses[i];

            if(auto it = address_cache.find(address); it != address_cache.end()) {
                callstack.push_back(it->second);
            } else {
                auto symbol = resolve_address(hProcess, address, options);
                address_cache.insert({address, symbol});
                callstack.push_back(std::move(symbol));
            }
        }
        std::reverse(callstack.begin(), callstack.end());
        resolved_samples.emplace_back(s.ts, callstack);
    }

    return resolved_samples;
}
