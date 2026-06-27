/**
 * @file dispatcher.cpp
 * @brief Consistent routing algorithm mappings for parallel event tracking execution.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#include "dispatcher.h"
#include <iostream>

Dispatcher::Dispatcher(ObjectPool& pool, size_t num_workers, size_t min_window_size, size_t max_window_size) 
 : pool_size(num_workers) 
{
    if (pool_size == 0) 
		pool_size = 4;

    std::cout << "[CONSUMER] Activating Partitioned Thread Pool with " << pool_size << " Workers.\n";
    for (size_t i = 0; i < pool_size; ++i) {
        workers.push_back(std::make_unique<Worker>(pool, min_window_size, max_window_size));
    }
}

void Dispatcher::route(DataPoint* point) {
    if (!point) return;

    // Sticky Hashing Mechanism: Forces specific tenant data profiles onto matching thread segments
    size_t hash_value = std::hash<std::string>{}(point->respondent_id);
    size_t worker_index = hash_value % pool_size;

    workers[worker_index]->enqueue(point);
}