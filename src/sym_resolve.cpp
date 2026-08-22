#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <format>
#include <chrono>
#include <vector>
#include <string>
#include <cstddef>

#include <buffer.h>

#include <windows.h>
#include <dbghelp.h>

#include <cstdio>
#include <fstream>
#include <stdexcept>

DWORD64 get_preferred_image_base(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
        throw std::runtime_error(
            "Could not open module: " + path
        );

    IMAGE_DOS_HEADER dos{};
    file.read(
        reinterpret_cast<char*>(&dos),
        sizeof(dos)
    );

    if (dos.e_magic != IMAGE_DOS_SIGNATURE)
        throw std::runtime_error("Invalid DOS header");

    file.seekg(dos.e_lfanew);

    DWORD signature{};
    file.read(
        reinterpret_cast<char*>(&signature),
        sizeof(signature)
    );

    if (signature != IMAGE_NT_SIGNATURE)
        throw std::runtime_error("Invalid PE header");

    IMAGE_FILE_HEADER fileHeader{};
    file.read(
        reinterpret_cast<char*>(&fileHeader),
        sizeof(fileHeader)
    );

    std::streampos optionalHeaderPos = file.tellg();

    WORD magic{};
    file.read(
        reinterpret_cast<char*>(&magic),
        sizeof(magic)
    );

    file.seekg(optionalHeaderPos);

    if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        IMAGE_OPTIONAL_HEADER64 header{};

        file.read(
            reinterpret_cast<char*>(&header),
            sizeof(header)
        );

        return header.ImageBase;
    }

    if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        IMAGE_OPTIONAL_HEADER32 header{};

        file.read(
            reinterpret_cast<char*>(&header),
            sizeof(header)
        );

        return header.ImageBase;
    }

    throw std::runtime_error(
        "Unknown PE optional-header format"
    );
}

std::string resolve_address(HANDLE hProcess, DWORD64 address) {
    if (address == 0)
        return "<null>";

    IMAGEHLP_MODULE64 info{};
    info.SizeOfStruct = sizeof(info);

    if (!SymGetModuleInfo64(
            hProcess,
            address,
            &info))
    {
        std::cerr << std::format(
            "No module for address 0x{:X}: error={}\n",
            address,
            GetLastError()
        );

        return "<unknown module>";
    }

    // Runtime base, e.g. 0x7FF613FB0000
    DWORD64 runtimeBase = info.BaseOfImage;

    // Prefer full loaded-image path.
    std::string modulePath;

    if (info.LoadedImageName[0] != '\0')
        modulePath = info.LoadedImageName;
    else
        modulePath = info.ImageName;

    if (modulePath.empty())
        return "<unknown module path>";

    DWORD64 preferredBase;

    try
    {
        preferredBase = get_preferred_image_base(modulePath);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return "<unknown>";
    }

    // Convert ASLR runtime address -> RVA
    DWORD64 rva =
        address - runtimeBase;

    // Convert RVA -> address addr2line expects.
    DWORD64 symbolAddress =
        preferredBase + rva;

    std::cout << std::format(
        "runtime=0x{:X} runtimeBase=0x{:X} "
        "rva=0x{:X} preferred=0x{:X} "
        "symbolAddress=0x{:X}\n",
        address,
        runtimeBase,
        rva,
        preferredBase,
        symbolAddress
    );

    std::string command = std::format(
        "addr2line -e \"{}\" -f -C 0x{:X}",
        modulePath,
        symbolAddress
    );

    FILE* pipe = _popen(command.c_str(), "r");

    if (!pipe)
        return "<addr2line failed>";

    char functionBuffer[1024]{};

    if (!fgets(
            functionBuffer,
            sizeof(functionBuffer),
            pipe))
    {
        _pclose(pipe);
        return "<unknown>";
    }

    _pclose(pipe);

    std::string functionName =
        functionBuffer;

    // Remove newline.
    while (!functionName.empty() &&
           (functionName.back() == '\n' ||
            functionName.back() == '\r'))
    {
        functionName.pop_back();
    }

    return functionName;
}

auto resolve_all_samples(HANDLE hProcess, RingBuffer samples) {
    std::vector<sampleSIM> resolved_samples{samples.get_sample_count()};

    for(Sample s : samples) {
        std::vector<std::string> callstack{};
        for(std::size_t i{}; i < s.frame_count; i++) {
            callstack.push_back(resolve_address(hProcess, s.addresses[i]));
        }
        resolved_samples.emplace_back(s.ts, callstack);
    }

    return resolved_samples;
}
