#include <iostream>
#include <format>

#include <sampler.h>
#include <sym_resolve.h>
#include <buffer.h>
#include <report.h>

std::string print_overhead(std::chrono::steady_clock::duration profiled, std::chrono::steady_clock::duration raw) {
    double overhead_percent = (
        static_cast<double>(std::chrono::duration<double, std::milli>(profiled).count()) / 
        static_cast<double>(std::chrono::duration<double, std::milli>(raw).count()) - 1.0) * 100.0;

    return std::format(
        "---------- Overhead ----------\n"
        "profiled_duration:     {:.3f} s\n"
        "raw_duration:          {:.3f} s\n"
        "overhead%:             {:.3f}%\n",
        std::chrono::duration<double>(profiled).count(),
        std::chrono::duration<double>(raw).count(),
        overhead_percent
    );
}

std::string summary(const profiler_options& options, const profiler_stats& stats)  {
    double overhead_percent = (
        static_cast<double>(std::chrono::duration<double, std::milli>(stats.profiled_time).count()) / 
        static_cast<double>(std::chrono::duration<double, std::milli>(stats.raw_time).count()) - 1.0) * 100.0;

    return std::format(
        "---------- Summary ----------\n"
        "Target:                {}\n"
        "Samples:               {} / {} max\n"
        "Stack Depth:           {:.1f} avg / {} max\n"
        "Frequency:             {} Hz\n"
        "Duration:              {:.3f} s\n"
        "Overhead:              {:.3f}%\n",
        options.executable,
        stats.valid_samples,
        options.max_samples,
        stats.avg_stack_depth,
        options.max_frames,
        options.hertz,
        std::chrono::duration<double>(stats.profiled_time).count(),
        overhead_percent
    );
}

void quick_sampling(const profiler_options& options, profiler_stats& stats)  {
    PROCESS_INFORMATION pi = launch_process(options, false);
    stats.raw_time = time_raw(pi, options);
    process_cleanup(pi);
}

auto detailed_sampling(const profiler_options& options, profiler_stats& stats)  {
    PROCESS_INFORMATION pi = launch_process(options);
    RingBuffer samples = run_sampler(pi, options);
    std::vector<sampleSIM> processed_samples = resolve_all_samples(pi.hProcess, samples, options, stats);
    process_cleanup(pi);

    auto events = generate_events(processed_samples, stats.profiled_time);
    quick_sampling(options, stats);
    auto times = generate_times(events);

    return times;
}

int main(int argc, char* argv[])
{
    auto options = parse_arguments(argc, argv);

    profiler_stats stats;
    if (options.verbose) {
        auto times = detailed_sampling(options, stats);
        std::cout << summary(options, stats);
        debug_times(times);

    } else {
        quick_sampling(options, stats);
        std::cout << std::chrono::duration<double>(stats.raw_time).count() << " s \n";
    }

    return 0;
}