#include "task_queue.h"

void TaskQueue::push( std::function<void()> task, int duration, int id )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_tasks.push( { task, duration, std::chrono::steady_clock::now(), id } );
    m_total_duration += duration;

    m_length_samples_sum += m_tasks.size();
    m_length_samples_count++;
}

bool TaskQueue::pop( std::function<void()>& task, std::chrono::steady_clock::time_point& added_time, int& id )
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if ( m_tasks.empty() ) return false;

    auto wrapper = m_tasks.front();
    m_tasks.pop();

    task = wrapper.task;
    added_time = wrapper.added_time;
    id = wrapper.id;
    m_total_duration -= wrapper.duration;

    m_length_samples_sum += m_tasks.size();
    m_length_samples_count++;

    return true;
}

size_t TaskQueue::get_total_duration() const
{
    return m_total_duration.load();
}

double TaskQueue::get_average_length() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    if ( m_length_samples_count == 0 ) return 0.0;
    return static_cast<double>( m_length_samples_sum ) / m_length_samples_count;
}

bool TaskQueue::empty() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_tasks.empty();
}