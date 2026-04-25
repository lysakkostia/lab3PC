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
    {
        std::lock_guard<std::mutex> lock( m_routing_mutex );
        if ( m_queues[0].get_total_duration() <= m_queues[1].get_total_duration() )
        {
            m_queues[0].push( task, duration, id );
        }
        else
        {
            m_queues[1].push( task, duration, id );
        }
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

            m_cv.wait( lock, [ this, &queue ] { return m_stop || (!queue.empty() && !m_paused); } );

            if ( m_immediate_stop ) return;

            auto wait_end = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = wait_end - wait_start;
            m_metrics.record_waiting_time( diff.count() );

            if ( m_stop && queue.empty() ) return;

            if ( !queue.pop( task, added_time, id ) ) continue;
        }

        auto exec_start = std::chrono::steady_clock::now();

        task();

        auto exec_end = std::chrono::steady_clock::now();

        if ( !m_immediate_stop )
        {
            std::chrono::duration<double> exec_diff = exec_end - exec_start;
            m_metrics.record_execution_time( exec_diff.count() );
            m_metrics.completed_tasks++;
        }
    }
}

void ThreadPool::terminate( bool immediate )
{
    {
        std::lock_guard<std::mutex> lock( m_cv_mutex );
        m_stop = true;
        m_immediate_stop = immediate;
        m_paused = false;
    }
    m_cv.notify_all();

    for ( auto& t : m_workers )
    {
        if ( t.joinable() ) t.join();
    }
}

void ThreadPool::pause()
{
    std::lock_guard<std::mutex> lock( m_cv_mutex );
    m_paused = true;
}

void ThreadPool::resume()
{
    {
        std::lock_guard<std::mutex> lock( m_cv_mutex );
        m_paused = false;
    }
    m_cv.notify_all();
}

bool ThreadPool::is_stopped() const
{
    std::lock_guard<std::mutex> lock( const_cast<std::mutex&>(m_cv_mutex) );
    return m_immediate_stop;
}

double ThreadPool::get_queue_average_length( int queue_index ) const
{
    if ( queue_index >= 0 && queue_index < 2 )
    {
        return m_queues[queue_index].get_average_length();
    }
    return 0.0;
}
