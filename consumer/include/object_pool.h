/**
 * @file object_pool.h
 * @brief Memory management pool for DataPoint structures to prevent heap fragmentation.
 *
 * Defines a thread-safe ObjectPool that pre-allocates DataPoint frames and recycles them
 * to support zero-allocation execution paths under high-throughput processing loads.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <vector>
#include <mutex>
#include <string>

/**
 * @struct DataPoint
 * @brief Memory frame representing parsed telemetry transaction data.
 */
struct DataPoint {
    std::string respondent_id; ///< Target tenant/respondent identifier.
    double metric_value{0.0};  ///< Numerical metric point.
    long long timestamp{0};    ///< Epoch record time in seconds.
};

/**
 * @class ObjectPool
 * @brief Thread-safe object recycling container for DataPoint instances.
 */
class ObjectPool {
private:
    std::vector<DataPoint*> pool; ///< Stack backing the active recycled pointers.
    std::mutex pool_mutex;        ///< Synchronization primitive guarding pool operations.
    size_t capacity;              ///< Pre-allocated baseline structure boundaries.

public:
    /**
     * @brief Constructs the ObjectPool and allocates memory frames.
     * @param initial_capacity Quantity of structures to initialize on startup.
     */
    explicit ObjectPool(size_t initial_capacity = 2000);
    
    /**
     * @brief Deconstructs the pool context and releases raw heap blocks safely.
     */
    ~ObjectPool();

    // Enforce linear ownership: inhibit copy behaviors
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    /**
     * @brief Acquires a clean pre-allocated DataPoint pointer instance.
     * @return DataPoint* Available data frame pointer ready for telemetry binding.
     */
    DataPoint* acquire();

    /**
     * @brief Recycles a processed data frame back into the reusable cache pool.
     * @param point Pointer reference instance targeted for clearing and recovery.
     */
    void release(DataPoint* point);
};

#endif // OBJECT_POOL_H