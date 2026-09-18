#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <cstddef>

#include <buffer.h>

enum class file_format {
    text,
    json,
    folded
};

struct profiler_options
{
    std::string executable;
    std::string pass_args;
    std::string output_file;
    file_format format = file_format::text;
    int frequency = 1000; // microseconds
    int hertz = 1000; // hertz
    std::size_t max_frames = 64;
    std::size_t max_samples = 100000;
    bool verbose = false;
    bool debug = false;

    void set_freq(int new_hertz) {
        hertz = new_hertz;
        frequency = 1000000 / new_hertz; 
    }
};

PROCESS_INFORMATION launch_process(const profiler_options& options, BOOL loadmodules=true);

RingBuffer run_sampler(PROCESS_INFORMATION pi, const profiler_options& options);

profiler_options parse_arguments(int argc, char* argv[]);

void process_cleanup(PROCESS_INFORMATION pi);

std::chrono::steady_clock::duration time_raw(PROCESS_INFORMATION pi, const profiler_options& options);