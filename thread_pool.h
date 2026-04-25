#pragma once
#include "task_queue.h"
#include "metrics.h"
#include <vector>
#include <thread>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    ThreadPool(Metrics& m);
    void initialize();

    void add_task( std::function<void()> task, int duration, int id );

    void terminate();

private:
    void worker_routine( TaskQueue& queue );

    TaskQueue m_queues[2];
    std::vector<std::thread> m_workers;
    std::condition_variable m_cv;
    std::mutex m_cv_mutex;
    bool m_stop = false;
    Metrics& m_metrics;
};