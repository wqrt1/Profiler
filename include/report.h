#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <buffer.h>

struct event {
    std::string type; // "start" or "end"
    std::chrono::steady_clock::time_point ts;
    std::string name;
};

struct frame {
    std::string name;
    std::chrono::steady_clock::time_point ts;
    std::chrono::steady_clock::duration self_time;
};

struct function_time {
    std::string name;
    std::chrono::steady_clock::duration total_time;
    std::chrono::steady_clock::duration self_time;

    bool operator<(const function_time& other) {
        return self_time < other.self_time;
    }
};

std::vector<function_time> generate_times(const std::vector<event>& events);

std::vector<event> generate_events(const std::vector<sampleSIM>& samples, std::chrono::steady_clock::duration& duration);

void debug_samples(const std::vector<sampleSIM>& samples);

void debug_events(const std::vector<event>& events);

void debug_times(const std::vector<function_time>& times);