#pragma once
#include <atomic>

struct Timer {
    inline static std::atomic<uint64_t> timer{1};

    static uint64_t getTime() {
        return timer.fetch_add(1);
    }
};

