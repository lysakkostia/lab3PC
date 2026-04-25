#pragma once
#include <queue>
#include <mutex>
#include <functional>
#include <atomic>
#include <chrono>

struct TaskWrapper {
    std::function<void()> task;
    int duration;
    std::chrono::steady_clock::time_point added_time;
    int id;
};

class TaskQueue {
public:
    TaskQueue() = default;

    void push( std::function<void()> task, int duration, int id );
    bool pop( std::function<void()>& task, std::chrono::steady_clock::time_point& added_time, int& id );
    bool empty() const;

    size_t get_total_duration() const;
    double get_average_length() const;

private:
    std::queue<TaskWrapper> m_tasks;
    mutable std::mutex m_mutex;
    std::atomic<size_t> m_total_duration{ 0 };

    size_t m_length_samples_sum{ 0 };
    size_t m_length_samples_count{ 0 };
};