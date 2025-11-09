#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <atomic>

// Clase para un pool de hilos que ejecuta tareas en paralelo
class ThreadPool {
public:
    // Constructor que inicia el pool con numThreads hilos
    explicit ThreadPool(size_t numThreads = 0);

    // Destructor que detiene el pool y une los hilos
    ~ThreadPool();

    // Encola una nueva tarea en el pool
    void enqueue(std::function<void()> task);

    // Espera a que todas las tareas encoladas se completen
    void waitForCompletion();

    // Obtiene el número de hilos en el pool
    size_t getThreadCount() const { return workers.size(); }

    // Deshabilitar copia y asignación
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    std::vector<std::thread> workers;           // Hilos worker
    std::queue<std::function<void()>> tasks;    // Cola de tareas
    std::mutex queueMutex;                      // Mutex para la cola
    std::condition_variable condition;           // Variable de condición
    std::atomic<bool> stop;                     // Flag para detener el pool
    std::atomic<size_t> activeTasks;            // Contador de tareas activas

    // Hilo worker que procesa tareas de la cola
    void workerThread();
};

#endif // THREADPOOL_H
