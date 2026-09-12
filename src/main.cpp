#include <sampler.h>
#include <sym_resolve.h>
#include <iostream>
#include <format>

#include <buffer.h>
#include <report.h>

int main(int argc, char* argv[])
{
    auto options = parse_arguments(argc, argv);

    PROCESS_INFORMATION pi = launch_process(options);
    RingBuffer samples = run_sampler(pi, options);
    std::vector<sampleSIM> processed_samples = resolve_all_samples(pi.hProcess, samples, options);
    process_cleanup(pi);

    pi = launch_process(options, false);
    auto raw_duration = time_raw(pi, options);
    process_cleanup(pi);

    std::chrono::steady_clock::duration profiled_duration{};
    auto events = generate_events(processed_samples, profiled_duration);
    auto times = generate_times(events);

    // auto overhead = profiled_duration - raw_duration;
    // double overhead_percent = (
    //     static_cast<double>(std::chrono::duration<double, std::milli>(profiled_duration).count()) / 
    //     static_cast<double>(std::chrono::duration<double, std::milli>(raw_duration).count()) - 1.0) * 100.0;

    //debug_samples(processed_samples);
    //debug_events(events);
    debug_times(times);
    std::cout << "---------- Overhead ----------\n";
    // std::cout << std::format("Overhead (ms): {}\nOverhead %: {}%", std::chrono::duration<double, std::milli>(overhead).count(), overhead_percent);
    std::cout << "profiled_duration: \t" << profiled_duration << '\n';
    std::cout << "raw_duration: \t\t" << raw_duration << '\n';

    return 0;
}