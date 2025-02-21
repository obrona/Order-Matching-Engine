#pragma once
#include <atomic>

struct Timer {
    std::atomic<uint64_t> timer = 1;

    uint64_t getTime() {
        return timer.fetch_add(1);
    }
};

Timer timer;