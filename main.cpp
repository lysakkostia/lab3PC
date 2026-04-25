#include "thread_pool.h"
#include "metrics.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <atomic>
#include <mutex>

std::mutex print_mutex;

int main() {

    Metrics stats;

    ThreadPool pool( stats );
    pool.initialize();

    std::atomic<int> global_task_id{1};

    {
        std::lock_guard<std::mutex> lock( print_mutex );
        std::cout << "--- Пул потоків запущено (2 черги, 4 потоки) ---\n" << std::endl;
    }

    std::vector<std::thread> producers;
    for ( int i = 0; i < 3; ++i )
    {
        producers.emplace_back( [ &pool, &global_task_id, i ]()
        {

            std::random_device rd;
            std::mt19937 gen( rd() );

            std::uniform_int_distribution<> dur_dist( 2, 15 ) ;
            std::uniform_int_distribution<> sleep_dist( 500, 1500 );

            for ( int j = 0; j < 5; ++j )
            {
                int duration = dur_dist( gen );

                int id = global_task_id.fetch_add( 1 );

                {
                    std::lock_guard<std::mutex> lock( print_mutex );
                    std::cout << "[Producer " << i << "] Додано задачу №" << id
                              << " (тривалість: " << duration << "с)\n";
                }

                pool.add_task( [duration, id ]()
                {
                    std::this_thread::sleep_for( std::chrono::seconds( duration ) );
                }
                , duration, id );

                std::this_thread::sleep_for( std::chrono::milliseconds( sleep_dist( gen ) ) );

            }
        } );
    }

    for (auto& t : producers) {
        t.join();
    }

    std::cout << "\n[System] Усі задачі додано. Очікування завершення...\n" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(40));

    pool.terminate();

    double total_wait = stats.total_waiting_time.load();
    size_t completed = stats.completed_tasks.load();

    {
        std::lock_guard<std::mutex> lock( print_mutex );
        std::cout << "========================================" << std::endl;
        std::cout << "РЕЗУЛЬТАТИ ТЕСТУВАННЯ" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Кількість створених потоків: 4" << std::endl;
        std::cout << "Виконано задач: " << completed << std::endl;
        std::cout << "Загальний час очікування воркерів: " << std::fixed << std::setprecision(2) << total_wait << "с" << std::endl;

        if (completed > 0) {
            std::cout << "Середній час очікування на 1 потік: " << (total_wait / 4.0) << "с" << std::endl;
            std::cout << "Середній час знаходження потоку в стані очікування (на 1 задачу): "
                      << (total_wait / completed) << "с" << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }
    return 0;
}