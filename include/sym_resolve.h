#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <cstddef>

#include <buffer.h>

struct profiler_stats {
private:
    std::size_t count{};

public:
    std::chrono::steady_clock::duration raw_time;
    std::chrono::steady_clock::duration profiled_time;
    std::size_t valid_samples{};
    double avg_stack_depth{};

    void running_avg(std::size_t stack_depth) {
        count++;
        avg_stack_depth += (stack_depth - avg_stack_depth) / count;
    }
};

std::vector<sampleSIM> resolve_all_samples(HANDLE hProcess, RingBuffer samples, const profiler_options& options, profiler_stats& stats);