#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <format>
#include <chrono>
#include <buffer.h>
#include <report.h>

static std::chrono::steady_clock::time_point final_ts{};

std::vector<event> generate_events(const std::vector<sampleSIM>& samples, std::chrono::steady_clock::duration& duration) {
    std::vector<event> events;

    auto first_events = samples[0].callstack;
    std::chrono::steady_clock::time_point first_ts = samples[0].ts;
    for (std::string function_name : first_events) {
        events.emplace_back("start", first_ts, function_name);
    }

    for (size_t i = 1; i < samples.size(); i++) {
        std::chrono::steady_clock::time_point ts = samples[i].ts;

        auto mismatch_pair = std::mismatch(
            samples[i-1].callstack.begin(), 
            samples[i-1].callstack.end(), 
            samples[i].callstack.begin(),
            samples[i].callstack.end()
        );
        for (auto it = samples[i-1].callstack.end(); it != mismatch_pair.first; ) {
            --it;
            events.emplace_back("end", ts, *it);
        }
        for (auto it = mismatch_pair.second; it != samples[i].callstack.end(); ++it) {
            events.emplace_back("start", ts, *it);
        }
    }

    auto final_ts = samples.back().ts;
    // clos e all ongoing functions
    const auto& final_stack = samples.back().callstack;
    for (auto it = final_stack.rbegin(); it != final_stack.rend(); ++it) {
        events.emplace_back("end", final_ts, *it);
    }

    duration = final_ts - first_ts;
    return events;
}

std::vector<function_time> generate_times(const std::vector<event>& events) {
    std::vector<function_time> times;
    std::stack<frame> callstack;
    auto last_ts = events.front().ts;

    for(const event& e : events) {
        auto dt = e.ts - last_ts;
        if (!callstack.empty()) {callstack.top().self_time += dt;}

        if (e.type == "start") {
            callstack.emplace(e.name, e.ts, std::chrono::steady_clock::duration::zero());
        } else if (e.type == "end") {
            const frame& f = callstack.top();
            times.emplace_back(f.name, e.ts - f.ts, f.self_time);
            callstack.pop();
        }
        last_ts = e.ts;
    }

    while (!callstack.empty()) {
        frame entry = callstack.top();
        times.emplace_back(entry.name, final_ts - entry.ts, callstack.top().self_time);
        callstack.pop();
    }

    return times;
}

void sort_times(std::vector<function_time>& times) {
    std::sort(times.rbegin(), times.rend());
}

void debug_samples(const std::vector<sampleSIM>& samples) {
    int i{};
    for (const sampleSIM& s : samples) {
        std::cout << std::format("[{}]", i++);
        for (const std::string func : s.callstack) {
            std::cout << " " << func;
        }
        std::cout << '\n';
    }
}

void debug_events(const std::vector<event>& events) {
    for (const event& e : events) {
        std::cout << e.name << " " << e.type << " at <time_point>" << '\n';
    }
}

void debug_times(const std::vector<function_time>& times)
{
    constexpr int NAME_WIDTH = 45;
    constexpr int TIME_WIDTH = 12;

    std::cout << std::format(
        "{:<{}} {:>{}} {:>{}}\n",
        "Function",   NAME_WIDTH,
        "Self (ms)",  TIME_WIDTH,
        "Total (ms)", TIME_WIDTH
    );

    std::cout << std::string(NAME_WIDTH + TIME_WIDTH * 2 + 2, '-') << '\n';

    for (const auto& t : times) {
        double self_ms = std::chrono::duration<double, std::milli>(t.self_time).count();

        double total_ms = std::chrono::duration<double, std::milli>(t.total_time).count();

        std::cout << std::format(
            "{:<{}} {:>{}.3f} {:>{}.3f}\n",
            t.name,   NAME_WIDTH,
            self_ms,  TIME_WIDTH,
            total_ms, TIME_WIDTH
        );
    }
}