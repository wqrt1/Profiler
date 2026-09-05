#include <sampler.h>
#include <sym_resolve.h>
#include <iostream>

#include <buffer.h>
#include <report.h>

int main(int argc, char* argv[])
{
    auto options = parse_arguments(argc, argv);

    PROCESS_INFORMATION pi = launch_process(options);

    RingBuffer samples = run_sampler(pi, options);

    std::vector<sampleSIM> processed_samples = resolve_all_samples(pi.hProcess, samples, options);

    kill_sampler(pi);

    auto events = generate_events(processed_samples);

    auto times = generate_times(events);

    debug_samples(processed_samples);
    
    debug_events(events);

    debug_times(times);

    return 0;
}