#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <chrono>

#include <buffer.h>

std::vector<sampleSIM> resolve_all_samples(HANDLE hProcess, RingBuffer samples);