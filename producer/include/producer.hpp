/**
 * @file producer.hpp
 * @brief Class declaration for the telemetry stream simulation Producer using modern C++.
 *
 * Defines the interface, random engine states, and statistical distribution parameters
 * using modern In-Class Member Initializers to decouple statistical specifications.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#ifndef PRODUCER_HPP
#define PRODUCER_HPP

#include <string>
#include <random>

/**
 * @class Producer
 * @brief Encapsulates telemetry stream simulation and network transmission logic.
 *
 * The Producer handles the state management of pseudo-random number generators,
 * statistical distributions for regular metrics and outliers, and manages the 
 * connection lifecycles to the Redis transmission layer.
 */
class Producer {
private:
    std::string host; ///< Redis instance host address destination.
    std::string port; ///< Redis instance target network port.

    // --- Modern C++ In-Class Member Initializers ---
    std::mt19937 gen{std::random_device{}()};                          ///< Mersenne Twister engine seeded via hardware entropy.
    std::normal_distribution<double> normal_dist{50.0, 5.0};           ///< Distribution for baseline normal data modeling (mean=50, stddev=5).
    std::bernoulli_distribution anomaly_trigger{0.05};                 ///< Distribution determining anomaly injection frequency (5% probability).
    std::uniform_real_distribution<double> anomaly_dist{100.0, 150.0}; ///< Distribution scaling anomaly outlier intensity bounds.
    std::uniform_int_distribution<int> respondent_dist{1, 100};       ///< Distribution allocating target mock tenant boundaries.

public:
    /**
     * @brief Constructs a new Producer instance and binds network contexts.
     * @param redis_host Target network string for the Redis instance.
     * @param redis_port Target network port configuration.
     */
    Producer(std::string redis_host, std::string redis_port);

    /**
     * @brief Starts the infinite loop data transmission pipeline.
     *
     * Connects to the Redis instance and executes a throttled loop emitting 
     * JSON telemetry events into the messaging subsystem channel.
     *
     * @throws sw::redis::Error if communication layer connection fails.
     * @throws std::exception for generic unexpected runtime failures.
     */
    void run();
};

#endif // PRODUCER_HPP