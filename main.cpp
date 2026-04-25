#include "thread_pool.h"
#include "metrics.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <string>

std::mutex print_mutex;

int main()
{
    Metrics stats;
    ThreadPool pool( stats );
    pool.initialize();

    std::atomic<int> global_task_id{ 1 };
    std::atomic<bool> immediate_stop_requested{ false };

    {
        std::lock_guard<std::mutex> lock( print_mutex );
        std::cout << "--- Пул потоків запущено (2 черги, 4 потоки) ---\n";
        std::cout << "КЕРУВАННЯ:\n";
        std::cout << " 'q' - Негайна зупинка (перервати все)\n";
        std::cout << " 'p' - Пауза\n";
        std::cout << " 'r' - Відновити (Resume)\n\n";
    }

    std::thread control_thread( [ & ]()
    {
        std::string cmd;
        while ( !immediate_stop_requested )
        {
            if ( std::getline( std::cin, cmd ) )
            {
                if ( cmd == "q" )
                {
                    {
                        std::lock_guard<std::mutex> lock( print_mutex );
                        std::cout << "\n[System] Ініційовано негайну зупинку!\n";
                    }
                    immediate_stop_requested = true;
                    break;
                }
                else if ( cmd == "p" )
                {
                    {
                        std::lock_guard<std::mutex> lock( print_mutex );
                        std::cout << "\n[System] ПАУЗА. Задачі в черзі очікують.\n";
                    }
                    pool.pause();
                }
                else if ( cmd == "r" )
                {
                    {
                        std::lock_guard<std::mutex> lock( print_mutex );
                        std::cout << "\n[System] Відновлення роботи.\n";
                    }
                    pool.resume();
                }
            }
        }
    } );
    control_thread.detach();

    std::vector<std::thread> producers;
    for ( int i = 0; i < 3; ++i )
    {
        producers.emplace_back( [ &pool, &global_task_id, &immediate_stop_requested, i ]()
        {
            std::random_device rd;
            std::mt19937 gen( rd() );
            std::uniform_int_distribution<> dur_dist( 2, 15 );
            std::uniform_int_distribution<> sleep_dist( 500, 1500 );

            for ( int j = 0; j < 5; ++j )
            {
                if ( immediate_stop_requested ) return;

                int duration = dur_dist( gen );
                int id = global_task_id.fetch_add( 1 );

                {
                    std::lock_guard<std::mutex> lock( print_mutex );
                    std::cout << "[Producer " << i << "] Додано задачу №" << id
                              << " (тривалість: " << duration << "с)\n";
                }

                pool.add_task( [ &immediate_stop_requested, duration, id ]()
                {
                    for ( int k = 0; k < duration * 10; ++k )
                    {
                        if ( immediate_stop_requested ) return;
                        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                    }
                }
                , duration, id );

                int sleep_time = sleep_dist( gen );
                for ( int s = 0; s < sleep_time; s += 100 )
                {
                    if ( immediate_stop_requested ) break;
                    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                }
            }
        } );
    }

    for ( auto& t : producers )
    {
        if ( t.joinable() ) t.join();
    }

    if ( !immediate_stop_requested )
    {
        {
            std::lock_guard<std::mutex> lock( print_mutex );
            std::cout << "\n[System] Продюсери завершили роботу.\n";
        }

        int expected_tasks = global_task_id.load() - 1;
        while ( !immediate_stop_requested && stats.completed_tasks.load() < expected_tasks )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
    }

    if ( immediate_stop_requested )
    {
        pool.terminate( true );
    }
    else
    {
        pool.terminate( false );
    }

    double total_wait = stats.total_waiting_time.load();
    size_t completed = stats.completed_tasks.load();

    {
        std::lock_guard<std::mutex> lock( print_mutex );
        std::cout << "========================================" << std::endl;
        std::cout << "РЕЗУЛЬТАТИ ТЕСТУВАННЯ" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Кількість створених потоків: 4" << std::endl;
        std::cout << "Виконано задач: " << completed << std::endl;
        std::cout << "Загальний час очікування воркерів: " << std::fixed << std::setprecision( 2 ) << total_wait << "с" << std::endl;

        if ( completed > 0 )
        {
            std::cout << "Середній час очікування на 1 потік: " << ( total_wait / 4.0 ) << "с" << std::endl;
            std::cout << "Середній час знаходження потоку в стані очікування (на 1 задачу): "
                      << ( total_wait / completed ) << "с" << std::endl;
        }
        std::cout << "Середня довжина 1-ї черги: " << pool.get_queue_average_length( 0 ) << std::endl;
        std::cout << "Середня довжина 2-ї черги: " << pool.get_queue_average_length( 1 ) << std::endl;
        std::cout << "========================================" << std::endl;
    }
    return 0;
}