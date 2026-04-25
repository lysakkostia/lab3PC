#pragma once
#include <atomic>

struct Metrics {
    std::atomic<size_t> completed_tasks{0};
    std::atomic<double> total_waiting_time{0.0};

    void record_waiting_time( double seconds )
    {
        double current = total_waiting_time.load();
        while ( !total_waiting_time.compare_exchange_weak( current, current + seconds ) );
    }
};