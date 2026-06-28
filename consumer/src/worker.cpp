/**
 * @file worker.cpp
 * @brief Implementation tracks for streaming analytics workers and statistical models.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#include "worker.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// Global log synchronization primitive to avoid multi-thread console log corruption
static std::mutex g_log_mutex;

RollingWindow::RollingWindow(size_t window_size) 
: min_size(window_size), 
  max_size(window_size * 2)
{

}

double RollingWindow::compute_z_score(double new_value, bool& can_evaluate) {

    if (window.size() < min_size) {
		window.push_back(new_value);
        can_evaluate = false;
        return 0.0;
    }
	
    can_evaluate = true;

    // (μ): compute the mean of the current historical baseline
    double sum = 0.0;
    for (double val : window) sum += val;
    double mean = sum / window.size();

    // (σ): compute the variance and deviation of the current historical baseline
    double variance_sum = 0.0;
    for (double val : window) {
        variance_sum += (val - mean) * (val - mean);
    }
    double stddev = std::sqrt(variance_sum / window.size());
	
    // calculate Z-score against the untainted historical baseline
    double z_score = 0.0;
    if (stddev > 0.00001) {
        z_score = std::abs(new_value - mean) / stddev;
    } else {
        // If baseline stddev is practically 0, any deviation from the mean is anomalous
        if (std::abs(new_value - mean) > 0.00001) {
            z_score = 99.99; // Force clear anomaly detection on a flatlined baseline
        }
    }	

    // slide the window to include the new point for the next incoming tick
    if (window.size() >= max_size) {
        window.pop_front();
    }
    window.push_back(new_value);
	
	return z_score;
}

Worker::Worker(ObjectPool& pool, size_t window_size) 
    : memory_pool(pool), 
      min_window_size(window_size), 
      max_window_size(window_size * 2),
      thread_handle([this](std::stop_token st) { this->process_loop(st); })
{

}

void Worker::enqueue(DataPoint* point) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push(point);
    }
    cv.notify_one(); 
}

void Worker::process_loop(std::stop_token st) {
    while (!st.stop_requested()) {
        DataPoint* point = nullptr;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, st, [this]() { return !task_queue.empty(); });
            
            if (task_queue.empty()) {
                if (st.stop_requested()) return;
                continue;
            }
            
            point = task_queue.front();
            task_queue.pop();
        }
        
        if (point) {
            execute_logic(point);
        }
    }
}

void Worker::execute_logic(DataPoint* point) {
    // If the mapping entry does not exist, instantiate a new RollingWindow instance
    if (local_windows.find(point->respondent_id) == local_windows.end()) {
        local_windows.emplace(point->respondent_id, RollingWindow(min_window_size));
    }
    
    bool can_evaluate = false;
    double z_score = local_windows[point->respondent_id].compute_z_score(point->metric_value, can_evaluate);
    
    if (!can_evaluate) {
        memory_pool.release(point);
        return;
    }
    
    // Lock logging frame to force clean, synchronous console outputs
    std::lock_guard<std::mutex> log_lock(g_log_mutex);
    std::cout << std::fixed << std::setprecision(2);
	
	auto thread_id = std::this_thread::get_id();
    
    if (z_score > 3.0) {
        // Anomaly detected alert log
        std::cout << "[Thread: " << thread_id << "]" "[" << point->timestamp << "] Data point: " << point->metric_value 
                  << " | Status: ANOMALY DETECTED! | Z-score: " << z_score 
                  << " | ALERT: Significant deviation detected. [ID: " << point->respondent_id << "]\n";
    } else {
        // Normal status info log
        std::cout << "[Thread: " << thread_id << "]" "[" << point->timestamp << "] Data point: " << point->metric_value 
                  << " | Status: OK | Z-score: " << z_score 
                  << " | [ID: " << point->respondent_id << "]\n";
    }

    memory_pool.release(point);
}