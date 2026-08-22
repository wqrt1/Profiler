#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <chrono>

#include <buffer.h>

struct profiler_options
{
    std::string executable;
    int frequency = 1000; // microseconds
    int maxFrames = 64;
    bool verbose = false;
};

PROCESS_INFORMATION launch_process(const profiler_options& options);

RingBuffer run_sampler(PROCESS_INFORMATION pi, profiler_options options);

profiler_options parse_arguments(int argc, char* argv[]);

void kill_sampler(PROCESS_INFORMATION pi);