#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <iostream>
#include <format>
#include <string>
#include <thread>
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <array>
#include <vector>

#include <buffer.h>

struct profiler_options {
    std::string executable;
    int frequency = 1000; // microseconds
    int maxFrames = 64;
    bool verbose = false;
};

struct ThreadSuspendGuard {
    HANDLE h;
    ThreadSuspendGuard(HANDLE h) : h(h) { SuspendThread(h); }
    ~ThreadSuspendGuard() { ResumeThread(h); }
};

void sample_once(HANDLE hProcess, HANDLE hThread, std::chrono::steady_clock::time_point ts, RingBuffer& samples) {
    std::array<DWORD64, MAX_FRAMES> addresses{};
    std::size_t count = 0;
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_FULL;

    {
        ThreadSuspendGuard guard(hThread); // suspend on construction

        if (!GetThreadContext(hThread, &ctx)) return;

        STACKFRAME64 frame{};
        frame.AddrPC.Offset    = ctx.Rip;
        frame.AddrPC.Mode      = AddrModeFlat;
        frame.AddrFrame.Offset = ctx.Rbp;
        frame.AddrFrame.Mode   = AddrModeFlat;
        frame.AddrStack.Offset = ctx.Rsp;
        frame.AddrStack.Mode   = AddrModeFlat;

        addresses[count++] = ctx.Rip;

        while (count < MAX_FRAMES)
        {
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &frame, &ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {break;}

            DWORD64 address = frame.AddrPC.Offset;

            if (address == 0) {break;}

            if (address == addresses[count - 1]) {break;}

            addresses[count++] = address;
        }
    } // guard destructor resumes the thread here

    std::cout << count << '\n'; //checking counts

    Sample sample{ts, addresses, count};
    samples.add(sample);
}

void sampler(int frequency, PROCESS_INFORMATION pi, RingBuffer& samples) {
    using clock = std::chrono::steady_clock;
    auto nextSample = clock::now();

    while (true) {
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {break;}

        sample_once(pi.hProcess, pi.hThread, clock::now(), samples);

        nextSample += std::chrono::microseconds(frequency);
        std::this_thread::sleep_until(nextSample);
    }
}

void load_modules(HANDLE hProcess, DWORD pid, const profiler_options& options)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        pid
    );

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << std::format(
            "Module snapshot failed: {}\n",
            GetLastError()
        );
        return;
    }

    MODULEENTRY32 module{};
    module.dwSize = sizeof(module);

    if (!Module32First(snapshot, &module))
    {
        std::cerr << std::format(
            "Module32First failed: {}\n",
            GetLastError()
        );

        CloseHandle(snapshot);
        return;
    }

    do
    {
        DWORD64 base = reinterpret_cast<DWORD64>(module.modBaseAddr);

        if(options.verbose) {
        std::cout << module.szModule
            << " 0x"
            << std::hex
            << base
            << " - 0x"
            << (base + module.modBaseSize)
            << '\n';};

        DWORD64 loaded = SymLoadModuleEx(
            hProcess,
            nullptr,
            module.szExePath,
            module.szModule,
            base,
            module.modBaseSize,
            nullptr,
            0
        );

        if (loaded == 0)
        {
            DWORD error = GetLastError();

            // Zero does not necessarily mean catastrophic
            // failure if module was already registered,
            // so print it for now.
            std::cerr << std::format(
                "SymLoadModuleEx: {} error {}\n",
                module.szModule,
                error
            );
        }

    } while (Module32Next(snapshot, &module));

    CloseHandle(snapshot);
}

PROCESS_INFORMATION launch_process(const profiler_options& options) {
    std::string processName{options.executable};
    PROCESS_INFORMATION pi{};
    STARTUPINFOA si{};
    si.cb = sizeof(si);

    BOOL success = CreateProcessA(
        nullptr,
        processName.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_SUSPENDED,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success)
    {
        throw std::runtime_error("Failed to lauch process");
    }

    SymSetOptions(
        SYMOPT_UNDNAME |
        SYMOPT_DEFERRED_LOADS |
        SYMOPT_DEBUG
    );

    if (!SymInitialize(pi.hProcess, nullptr, FALSE))
    {
        throw std::runtime_error(std::format("SymInitialize failed: {}" , GetLastError()));
    }

    // Start the target.
    if (ResumeThread(pi.hThread) == static_cast<DWORD>(-1))
    {
        std::cerr << "ResumeThread failed: " << GetLastError() << '\n';
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    load_modules(pi.hProcess, pi.dwProcessId, options);

    return pi;
}

RingBuffer run_sampler(PROCESS_INFORMATION pi, const profiler_options& options) {
    RingBuffer samples(MAX_SAMPLES);
    sampler(options.frequency, pi, samples);

    // load_modules(pi.hProcess, pi.dwProcessId);
    WaitForSingleObject(pi.hProcess, INFINITE);

    return samples;
}

void kill_sampler(PROCESS_INFORMATION pi) {
    SymCleanup(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

profiler_options parse_arguments(int argc, char* argv[]) {
    profiler_options options;

    if (argc < 2)
    {
        throw std::runtime_error("Usage: myprofiler <executable>");
    }
    options.executable = argv[1];

    for(int i{2}; i < argc; i++)
    {
        std::string arg = argv[i];

        if(arg == "--freq")
        {
            if(i + 1 >= argc) {throw std::runtime_error("--freq requires a value (microseconds)");}
            i++;
            options.frequency = std::stoi(argv[i]);
        } else if(arg == "--max-frames")
        {
            if(i + 1 >= argc) {throw std::runtime_error("--max-frames requires a value");}
            i++;
            options.maxFrames = std::stoi(argv[i]);
        } else if(arg == "--verbose")
        {
            options.verbose = true;
        }
    }

    return options;
}

// int main(int argc, char* argv[])
// {
//     auto options = parse_arguments(argc, argv);

//     RingBuffer samples = launch_process(options.executable, options.frequency);

//     std::cout << &samples;
//     return 0;
// }
