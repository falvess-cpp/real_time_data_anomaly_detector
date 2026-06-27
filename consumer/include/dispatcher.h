/**
 * @file dispatcher.h
 * @brief Dispatching coordinator implementing non-blocking consistent routing metrics.
 *
 * Allocates computation loads across multiple background workers based on hashing keys.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "worker.h"
#include <vector>
#include <memory>

/**
 * @class Dispatcher
 * @brief Coordinates task mapping distributions using modular hash routing techniques.
 */
class Dispatcher {
private:
    std::vector<std::unique_ptr<Worker>> workers; ///< Active worker threads pool instances array.
    size_t pool_size{4};                         ///< Bound sizing driving operational thread splits.

public:
    /**
     * @brief Allocates execution splits and initializes thread execution lanes.
     * @param pool Reference context directing structure recycle frameworks.
     * @param num_workers Operational thread lane boundaries.
     * @param window_size Data analysis sliding window limits configuration.
     */
    Dispatcher(ObjectPool& pool, size_t num_workers, size_t min_window_size, size_t max_window_size);
    
    ~Dispatcher() = default;

    /**
     * @brief Resolves key hash configurations and passes data targets to dedicated slots.
     * @param point Processed structural metrics frame reference pointer.
     */
    void route(DataPoint* point);
};

#endif // DISPATCHER_H