#pragma once

#include <windows.h>
#include <chrono>
#include <array>

constexpr std::size_t MAX_FRAMES{64};
constexpr std::size_t MAX_SAMPLES{100000};

class Sample {
public:
    std::chrono::steady_clock::time_point ts{};
    std::array<DWORD64, MAX_FRAMES> addresses{};
    std::size_t frame_count{};

    Sample() = default;

    Sample(const std::array<DWORD64, MAX_FRAMES>& addresses, std::chrono::steady_clock::time_point ts, std::size_t frame_count) 
        : addresses(addresses), ts(ts), frame_count(frame_count) {}

    std::size_t getFrameCount() const {
        return frame_count;
    }

    const auto& getAddresses() const {
        return addresses;
    }
};

class RingBuffer {
private:
    std::size_t size;
    std::vector<Sample> samples;

    std::size_t indexAdd{};
    std::size_t indexPop{};

    typedef Sample* iterator;
    typedef const Sample* const_iterator;

public:
    explicit RingBuffer(std::size_t count)
        : size(count), samples(count) {}

    std::size_t get_sample_count() {
        return indexAdd - indexPop;
    }

    void add(Sample sample) {
        samples[indexAdd % size] = sample;
        indexAdd++;
    }

    bool empty() const {
        return indexPop == indexAdd;
    }

    Sample pop() {
        if (empty())
            throw std::runtime_error("RingBuffer empty or overflow");

        Sample sample = samples[indexPop % size];
        ++indexPop;

        return sample;
    }

    iterator begin() { return &samples[indexPop]; }
    const_iterator begin() const { return &samples[indexPop]; }
    iterator end() { return &samples[indexAdd]; }
    const_iterator end() const { return &samples[indexAdd]; }
};

struct sampleSIM {
    std::chrono::steady_clock::time_point ts;
    std::vector<std::string> callstack;
};