#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t numThreads) 
    : stop(false), activeTasks(0) {
    
    // Si no se especifica, usar hardware_concurrency
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // Fallback
    }

    // Crear los hilos worker
    workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) {
            throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
        }
        tasks.push(std::move(task));
        activeTasks++;
    }
    condition.notify_one();
}

void ThreadPool::waitForCompletion() {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (tasks.empty() && activeTasks == 0) {
            break;
        }
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { 
                return stop || !tasks.empty(); 
            });
            
            if (stop && tasks.empty()) {
                return;
            }
            
            if (!tasks.empty()) {
                task = std::move(tasks.front());
                tasks.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Error en worker thread: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Error desconocido en worker thread" << std::endl;
            }
            activeTasks--;
        }
    }
}
