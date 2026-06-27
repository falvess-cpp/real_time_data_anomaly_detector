/**
 * @file worker.h
 * @brief Pipeline calculation worker engine executing bounded sliding window mathematics.
 *
 * Defines the computational mechanics for extracting rolling variance, means, and testing
 * raw incoming signals against standard deviation threshold equations within concurrent lanes.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#ifndef WORKER_H
#define WORKER_H

#include "object_pool.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <string>
#include <deque>

/**
 * @class RollingWindow
 * @brief Sliding Window implementation - it stores the datasets continuasly using FIFO.
 * Once it reaches the min size, it starts the statistics evaluation 
 */
class RollingWindow {
private:
    std::deque<double> window;               ///< Double-ended queue for store and track the datasets.
	size_t min_size{50};                     ///< Minimum window size to start evaluating datasets.
    size_t max_size{100};                    ///< Maximum window size to hold the datasets
    
public:
    /**
     * @brief Constructs a rolling window analyzer.
     * @param size Boundaries defining sliding limits.
     */
    explicit RollingWindow(size_t min_size = 50, size_t max_size = 100);

    /**
     * @brief Shifts data limits, updates variance records, and evaluates Z-score indicators.
     * @param new_value Incoming metrics observation factor.
     * @param can_evaluate Output parameter indicating if baseline limits are satisfied.
     * @return double Calculated Z-score indicator metric.
     */
    double compute_z_score(double new_value, bool& can_evaluate);
};

/**
 * @class Worker
 * @brief Independent execution thread for processing assigned datasets.
 */
class Worker {
private:
    ObjectPool& memory_pool;                ///< Injected object pool.
    size_t min_window_size{50};             ///< Min limit window parameter passed down into internal RollingWindow.
	size_t max_window_size{100};            ///< Max limit window parameter passed down into internal RollingWindow.
    std::queue<DataPoint*> task_queue;      ///< Thread local queue backing tasks mapped to this execution target.
    std::mutex queue_mutex;                  ///< Serialization control guarding enqueue/dequeue blocks.
    std::condition_variable_any cv;          ///< Notification coordinator driving thread sleep states.
	std::jthread thread_handle;             ///< Managed thread handle context providing stop-token automation.
    
    // Isolated thread-local - Zero lock contention for looking up windows
    std::unordered_map<std::string, RollingWindow> local_windows;

    /**
     * @brief Execution loop processing incoming items off queue contexts.
     * @param st Token context monitoring parent interrupt signaling hooks.
     */
    void process_loop(std::stop_token st);

    /**
     * @brief Parses metrics against mapping profiles and logs target evaluation results.
     * @param point Target data record frame containing metrics profiles.
     */
    void execute_logic(DataPoint* point);

public:
    /**
     * @brief Prepares execution frameworks and boots active thread handlers.
     * @param pool Reference context directing structural recycle frameworks.
     * @param min_window_size Min onfiguration limit driving sliding data windows.
	 * @param max_window_size Max Configuration limit driving sliding data windows.
     */
    Worker(ObjectPool& pool, size_t min_window_size, size_t max_window_size);
    
    ~Worker() = default;

    /**
     * @brief Appends task pointers into worker sequences and awakens execution threads.
     * @param point Structural target containing event payloads.
     */
    void enqueue(DataPoint* point);
};

#endif // WORKER_H