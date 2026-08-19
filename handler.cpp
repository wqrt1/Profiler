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

constexpr int MAX_FRAMES = 64;

class RingBuffer {
public:
    explicit RingBuffer(std::size_t count)
        : size(count), pointers_(new std::string[count] {}) {}

    ~RingBuffer() {
        delete[] pointers_;
    }

    void add(std::string address)
    {
        pointers_[indexAdd % size] = address;
        indexAdd++;
    }

    std::string pop()
    {
        auto ptr{pointers_[indexPop % size]};
        indexPop++;
        return ptr;
    }

private:
    std::size_t size;
    std::string* pointers_;
    std::size_t indexAdd{};
    std::size_t indexPop{};
};

std::string resolve_address(HANDLE hProcess, DWORD64 address)
{
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};

    auto* symbol =
        reinterpret_cast<SYMBOL_INFO*>(buffer);

    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;

    if (!SymFromAddr(
            hProcess,
            address,
            &displacement,
            symbol))
    {
        std::cerr
            << "SymFromAddr failed for 0x"
            << std::hex << address
            << " error = "
            << std::dec << GetLastError()
            << '\n';

        return std::string("<unknown>");
    }
    std::cout << symbol->Name << '\n';
    return symbol->Name;
}

void push_sample_to_ring_buffer(HANDLE hProcess, DWORD64 addresses[], int count) 
{
    RingBuffer buffer{static_cast<std::size_t>(MAX_FRAMES)};
    for (std::size_t i{}; i < count; i++)
    {
        buffer.add(resolve_address(hProcess, addresses[i]));
        // std::cout << "pushToRB: " << addresses[i] << '\n';
    }

    // std::cout << "sample collected\n";
}

struct ThreadSuspendGuard {
    HANDLE h;
    ThreadSuspendGuard(HANDLE h) : h(h) { SuspendThread(h); }
    ~ThreadSuspendGuard() { ResumeThread(h); }
};

void sample_once(HANDLE hProcess, HANDLE hThread) {
    DWORD64 addresses[MAX_FRAMES];
    int count = 0;
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_FULL;

    {
        ThreadSuspendGuard guard(hThread);   // suspend on construction

        if (!GetThreadContext(hThread, &ctx)) return;

        STACKFRAME64 frame = {};
        frame.AddrPC.Offset    = ctx.Rip;
        frame.AddrPC.Mode      = AddrModeFlat;
        frame.AddrFrame.Offset = ctx.Rbp;
        frame.AddrFrame.Mode   = AddrModeFlat;
        frame.AddrStack.Offset = ctx.Rsp;
        frame.AddrStack.Mode   = AddrModeFlat;

        addresses[count++] = reinterpret_cast<DWORD64>(ctx.Rip);

        while (count < MAX_FRAMES)
        {
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), hThread, &frame, &ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) break;

            DWORD64 address = frame.AddrPC.Offset;

            if (address == 0) {break;}

            if (address == addresses[count - 1]) {continue;}

            addresses[count++] = reinterpret_cast<DWORD64>(frame.AddrPC.Offset);
        }
    } // guard destructor resumes the thread here

    push_sample_to_ring_buffer(hProcess, addresses, count);  // do this after resume, off the critical path
}

void sampler(int frequency, PROCESS_INFORMATION pi) 
{
    using clock = std::chrono::steady_clock;
    auto nextSample = clock::now();

    while (true)
    {
        sample_once(pi.hProcess, pi.hThread);

        nextSample += std::chrono::microseconds(frequency);

        if (WaitForSingleObject(pi.hThread, 0) == WAIT_OBJECT_0) {break;}
        std::this_thread::sleep_until(nextSample);
    }
}

void load_modules(HANDLE hProcess, DWORD pid)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        pid
    );

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Module snapshot failed: " << GetLastError() << '\n';
        return;
    }

    MODULEENTRY32 module{};
    module.dwSize = sizeof(module);

    if (!Module32First(snapshot, &module))
    {
        CloseHandle(snapshot);
        return;
    }

    do
    {
        DWORD64 base =
            reinterpret_cast<DWORD64>(
                module.modBaseAddr
            );

        std::cout << module.szModule
            << " 0x"
            << std::hex
            << base
            << " - 0x"
            << (base + module.modBaseSize)
            << '\n';

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
            std::cerr
                << "SymLoadModuleEx: "
                << module.szModule
                << " error "
                << std::dec
                << error
                << '\n';
        }

    } while (Module32Next(snapshot, &module));

    CloseHandle(snapshot);
}

void launch_process(std::string name, int freq)
{
    std::string processName{name};
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
        std::cerr << "Failed to lauch process\n";
    }

    SymSetOptions(
        SYMOPT_UNDNAME |
        SYMOPT_DEFERRED_LOADS |
        SYMOPT_DEBUG
    );

    if (!SymInitialize(pi.hProcess, nullptr, FALSE))
    {
        std::cerr << "SymInitialize failed: " << GetLastError() << '\n';
    }

    // Start the target.
    if (ResumeThread(pi.hThread) == static_cast<DWORD>(-1))
    {
        std::cerr << "ResumeThread failed: "
                  << GetLastError() << '\n';
    }

    DWORD64 base = SymLoadModuleEx(
        pi.hProcess,
        nullptr,
        processName.c_str(), // executable path
        nullptr,
        0,
        0,
        nullptr,
        0
    );

    if (base == 0)
    {
        DWORD err = GetLastError();

        if (err != ERROR_SUCCESS)
        {
            std::cerr << "SymLoadModuleEx failed: " << err << '\n';
        }
    }
    else
    {
        std::cout << "Loaded symbols at base: 0x" << std::hex << base << '\n';
    }
    //read from thread
    sampler(freq, pi);

    load_modules(pi.hProcess, pi.dwProcessId);
    WaitForSingleObject(pi.hProcess, INFINITE);

    std::cout << "Target exited.\n";

    SymCleanup(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

struct profiler_options
{
    std::string executable;
    int frequency = 1000; // microseconds
    int maxFrames = 64;
    bool verbose = false;
};

profiler_options parse_arguments(int argc, char* argv[])
{
    profiler_options options;

    if (argc < 2)
    {
        std::cout <<  "Usage: myprofiler <executable>\n";
    }
    options.executable = argv[1];

    for(int i{2}; i < argc; i++)
    {
        if(argv[i] == "--freq")
        {
            if(i + 1 > argc) {throw std::runtime_error("--freq requires a value (microseconds)");}
            i++;
            options.frequency = atoi(argv[i]);
        } else if(argv[i] == "--max-frames")
        {
            if(i + 1 > argc) {throw std::runtime_error("--max-frames requires a value");}
            i++;
            options.maxFrames = atoi(argv[i]);
        } else if(argv[i] == "--verbose")
        {
            options.verbose = true;
        }
    }

    return options;
}

int main(int argc, char* argv[])
{
    auto options = parse_arguments(argc, argv);

    launch_process(options.executable, options.frequency);

    return 0;
}
