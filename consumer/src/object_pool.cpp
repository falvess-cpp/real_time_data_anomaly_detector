/**
 * @file object_pool.cpp
 * @brief Implementation patterns for the thread-safe structure recycling memory pool.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#include "object_pool.h"

ObjectPool::ObjectPool(size_t initial_capacity) : capacity(initial_capacity) {
    std::lock_guard<std::mutex> lock(pool_mutex);
    pool.reserve(capacity);
    for (size_t i = 0; i < capacity; ++i) {
        pool.push_back(new DataPoint());
    }
}

ObjectPool::~ObjectPool() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    for (DataPoint* point : pool) {
        delete point;
    }
    pool.clear();
}

DataPoint* ObjectPool::acquire() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    if (pool.empty()) {
        return new DataPoint();
    }
    DataPoint* point = pool.back();
    pool.pop_back();
    return point;
}

void ObjectPool::release(DataPoint* point) {
    if (!point) return;
	
	// restore default data
	point->respondent_id.clear();
	point->metric_value = 0.0;
	point->timestamp = 0;
		
	std::lock_guard<std::mutex> lock(pool_mutex);
	if (pool.size() >= capacity) {
		delete point;
	} else {    
		pool.push_back(point);
	}
}