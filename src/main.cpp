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

    std::vector<sampleSIM> processed_samples = resolve_all_samples(pi.hProcess, samples);

    kill_sampler(pi);

    auto events = generate_events(processed_samples);

    auto times = generate_times(events);

    debug_times(times);

    // for(sampleSIM s : processed_samples) {
    //     for(std::string a : s.callstack) {
    //         std::cout << a << ' ';
    //     }
    //     std::cout << '\n';
    // }

    // std::cout << &samples;
    return 0;
}