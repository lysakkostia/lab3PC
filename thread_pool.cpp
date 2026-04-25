#include "thread_pool.h"
#include <iostream>
#include <chrono>

ThreadPool::ThreadPool(Metrics& m) : m_metrics(m) {}

void ThreadPool::initialize()
{
    for ( int i = 0; i < 2; ++i )
    {
        for ( int j = 0; j < 2; ++j )
        {
            m_workers.emplace_back( &ThreadPool::worker_routine, this, std::ref( m_queues[i] ) );
        }
    }
}

void ThreadPool::add_task( std::function<void()> task, int duration, int id )
{
    if ( m_queues[0].get_total_duration() <= m_queues[1].get_total_duration() )
    {
        m_queues[0].push( task, duration, id );
    }
    else
    {
        m_queues[1].push( task, duration, id );
    }
    m_cv.notify_all();
}

void ThreadPool::worker_routine( TaskQueue& queue )
{
    while ( true )
    {
        std::function<void()> task;
        std::chrono::steady_clock::time_point added_time;
        int id;

        {
            std::unique_lock<std::mutex> lock( m_cv_mutex );
            auto wait_start = std::chrono::steady_clock::now();

            m_cv.wait( lock, [ this, &queue ] { return m_stop || !queue.empty(); } );

            auto wait_end = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = wait_end - wait_start;
            m_metrics.record_waiting_time( diff.count() );

            if ( m_stop && queue.empty() ) return;

            if ( !queue.pop( task, added_time, id ) ) continue;
        }

        task();
        m_metrics.completed_tasks++;
    }
}

void ThreadPool::terminate()
{
    {
        std::lock_guard<std::mutex> lock( m_cv_mutex );
        m_stop = true;
    }
    m_cv.notify_all();
    for ( auto& t : m_workers )
    {
        if ( t.joinable() ) t.join();
    }
}