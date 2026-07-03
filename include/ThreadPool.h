#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    // Initialize the thread pool with a specific number of worker threads
    explicit ThreadPool(size_t numThreads);
    
    // Destructor stops all threads and joins them
    ~ThreadPool();

    // Prevent copying
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Enqueue a new task to be executed by a worker thread
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    
    // Atomic flag to signal threads to stop when the pool is destroyed
    std::atomic<bool> m_stop;

    // The infinite loop that each worker thread runs
    void workerLoop();
};

#endif // THREADPOOL_H
