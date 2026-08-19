#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stack>
#include <format>

static double final_ts{};

struct sample {
    double ts;
    std::vector<std::string> callstack;
};

struct event {
    std::string type; // "start" or "end"
    double ts;
    std::string name;
};

struct frame {
    std::string name;
    double ts;
    double self_time;
};

struct function_time {
    std::string name;
    double total_time;
    double self_time;

    bool operator<(const function_time& other) {
        return self_time < other.self_time;
    }
};

auto generate_events(const std::vector<sample>& samples) {
    std::vector<event> events;

    auto first_events = samples[0].callstack;
    double first_ts = samples[0].ts;
    for (std::string function_name : first_events) {
        events.emplace_back("start", first_ts, function_name);
    }

    for (size_t i = 1; i < samples.size(); i++) {
        double ts = samples[i].ts;

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
    return events;
}

auto generate_times(const std::vector<event>& events) {
    std::vector<function_time> times;
    std::stack<frame> callstack;
    double last_ts = events.front().ts;

    for(const event& e : events) {
        double dt = e.ts - last_ts;
        if (!callstack.empty()) {callstack.top().self_time += dt;}

        if (e.type == "start") {
            callstack.emplace(e.name, e.ts, 0);
        } else if (e.type == "end") {
            times.emplace_back(e.name, e.ts - callstack.top().ts, callstack.top().self_time);
            callstack.pop();
        }
        last_ts = e.ts;
    }

    final_ts = events.back().ts;
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

void debug_events(const std::vector<event>& events) {
    for (const event& e : events) {
        std::cout << e.type << " " << e.name << " at " << e.ts << std::endl;
    }
}

void debug_times(const std::vector<function_time>& times) {
    std::cout << "Function\tTotal\tSelf\t%" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    for (const function_time& t : times) {
        std::cout << std::format("{}\t\t{}\t{}\t{}", t.name, t.total_time, t.self_time, static_cast<int>((t.self_time/final_ts)*100)) << std::endl;
    }
}

std::vector<sample> samples = {
    {0.0, {"main", "func1", "func2"}},
    {1.0, {"main", "func1", "f3"}},
    {2.0, {"main", "xx"}},
    {3.0, {"main", "func1", "func2"}},
    {4.0, {"main", "xxx"}}
};

std::vector<sample> samples_long = { // ai gen
    {0.0, {"main"}},
    {0.5, {"main", "init"}},
    {1.0, {"main", "init", "load"}},
    {1.5, {"main", "init", "load"}},
    {2.0, {"main", "init"}},
    {2.5, {"main", "run"}},
    {3.0, {"main", "run", "step1"}},
    {3.5, {"main", "run", "step1", "calc"}},
    {4.0, {"main", "run", "step1"}},
    {4.5, {"main", "run", "step2"}},
    {5.0, {"main", "run", "step2", "calc"}},
    {5.5, {"main", "run", "step2"}},
    {6.0, {"main", "run"}},
    {6.5, {"main", "cleanup"}},
    {7.0, {"main"}},

    {7.5, {"main", "funcA"}},
    {8.0, {"main", "funcA", "inner1"}},
    {8.5, {"main", "funcA", "inner2"}},
    {9.0, {"main", "funcA"}},
    {9.5, {"main", "funcB"}},
    {10.0, {"main", "funcB", "inner"}},
    {10.5, {"main", "funcB"}},
    {11.0, {"main"}},

    {11.5, {"main", "loop"}},
    {12.0, {"main", "loop", "iter1"}},
    {12.5, {"main", "loop", "iter1", "work"}},
    {13.0, {"main", "loop", "iter1"}},
    {13.5, {"main", "loop", "iter2"}},
    {14.0, {"main", "loop", "iter2", "work"}},
    {14.5, {"main", "loop", "iter2"}},
    {15.0, {"main", "loop"}},
    {15.5, {"main"}},

    {16.0, {"main", "io"}},
    {16.5, {"main", "io", "read"}},
    {17.0, {"main", "io"}},
    {17.5, {"main", "io", "write"}},
    {18.0, {"main", "io"}},
    {18.5, {"main"}},

    {19.0, {"main", "compute"}},
    {19.5, {"main", "compute", "phase1"}},
    {20.0, {"main", "compute", "phase1", "task"}},
    {20.5, {"main", "compute", "phase1"}},
    {21.0, {"main", "compute", "phase2"}},
    {21.5, {"main", "compute", "phase2", "task"}},
    {22.0, {"main", "compute", "phase2"}},
    {22.5, {"main", "compute"}},
    {23.0, {"main"}},

    {23.5, {"main", "finalize"}},
    {24.0, {"main"}}
};

int main() {
    auto events = generate_events(samples_long);
    auto times = generate_times(events);
    sort_times(times);
    
    return 0;
}