cmake_minimum_required(VERSION 3.14)
project(ThreadPoolLab)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(ThreadPoolLab
        main.cpp
        thread_pool.cpp
        task_queue.cpp
        main.cpp
        task_queue.cpp
        task_queue.h
        metrics.h
)

target_link_libraries(ThreadPoolLab PRIVATE Threads::Threads)