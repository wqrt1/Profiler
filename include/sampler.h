#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <chrono>

#include <buffer.h>

struct profiler_options
{
    std::string executable;
    std::string pass_args;
    int frequency = 1000; // microseconds
    int maxFrames = 64;
    bool verbose = false;
};

PROCESS_INFORMATION launch_process(const profiler_options& options, BOOL loadmodules=true);

RingBuffer run_sampler(PROCESS_INFORMATION pi, const profiler_options& options);

profiler_options parse_arguments(int argc, char* argv[]);

void process_cleanup(PROCESS_INFORMATION pi);

std::chrono::steady_clock::duration time_raw(PROCESS_INFORMATION pi, const profiler_options& options);