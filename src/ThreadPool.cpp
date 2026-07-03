#include "ThreadPool.h"
#include "Logger.h"

ThreadPool::ThreadPool(size_t numThreads) : m_stop(false) {
    // Launch 'numThreads' workers
    for (size_t i = 0; i < numThreads; ++i) {
        // Each worker thread executes the workerLoop method
        m_workers.emplace_back(&ThreadPool::workerLoop, this);
    }
    LOG_INFO("ThreadPool initialized with " + std::to_string(numThreads) + " threads.");
}

ThreadPool::~ThreadPool() {
    {
        // Lock the queue to safely set the stop flag
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    
    // Wake up all threads so they can check the m_stop flag and exit
    m_condition.notify_all();

    // Join all threads to ensure they finish execution before destruction
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    LOG_INFO("ThreadPool destroyed. All threads joined.");
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        // Lock the queue to safely push the new task
        std::unique_lock<std::mutex> lock(m_queueMutex);
        
        if (m_stop) {
            throw std::runtime_error("Cannot enqueue on a stopped ThreadPool");
        }
        
        m_tasks.push(std::move(task));
    }
    
    // Wake up exactly one waiting thread to pick up this task
    m_condition.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            // Lock the mutex before checking the queue or waiting
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            // Wait until a task is available OR the pool is stopped.
            // The lambda acts as a predicate to protect against spurious wakeups.
            m_condition.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });

            // If the pool is stopping and the queue is empty, exit the thread loop
            if (m_stop && m_tasks.empty()) {
                return;
            }

            // Pop the task from the queue
            task = std::move(m_tasks.front());
            m_tasks.pop();
        } // The lock is automatically released here

        // Execute the task outside the lock so other threads can still access the queue
        if (task) {
            task();
        }
    }
}
