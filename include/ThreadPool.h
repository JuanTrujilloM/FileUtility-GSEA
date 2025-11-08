#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <atomic>

/**
 * @brief Thread pool para ejecutar tareas en paralelo
 * 
 * Gestiona un conjunto de hilos worker que procesan tareas de una cola.
 * Número de hilos = hardware_concurrency() del sistema.
 */
class ThreadPool {
public:
    /**
     * @brief Constructor que inicializa el thread pool
     * @param numThreads Número de hilos (por defecto: hardware_concurrency)
     */
    explicit ThreadPool(size_t numThreads = 0);

    /**
     * @brief Destructor que espera a que terminen todas las tareas
     */
    ~ThreadPool();

    /**
     * @brief Añade una tarea a la cola para ser ejecutada
     * @param task Función a ejecutar
     */
    void enqueue(std::function<void()> task);

    /**
     * @brief Espera a que todas las tareas pendientes terminen
     */
    void waitForCompletion();

    /**
     * @brief Obtiene el número de hilos del pool
     * @return Número de hilos worker
     */
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

    /**
     * @brief Función que ejecuta cada worker
     */
    void workerThread();
};

#endif // THREADPOOL_H
