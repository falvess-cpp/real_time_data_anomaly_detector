/**
 * @file producer.hpp
 * @brief Class declaration for the real-time telemetry stream Producer.
 *
 * Defines the interface and architectural boundaries for the data ingestion engine.
 * This component acts as an autonomous source responsible for simulating concurrent
 * multi-tenant workloads, encapsulating the rules for steady-state traffic, and
 * managing controlled outlier injections before data transmission.
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

    std::mt19937 gen{std::random_device{}()}; ///< Standard pseudo-random engine seeded by the OS system clock/hardware.
    
	std::normal_distribution<double> normal_dist{50.0, 5.0}; ///< Generates baseline 'healthy' metrics centered around 50.0 (with a typical variance of +/- 5.0).
    
	std::bernoulli_distribution anomaly_trigger{0.05}; ///< Acts as a biased coin flip to trigger an spike/outlier exactly 5% of the time.
    
	std::uniform_real_distribution<double> anomaly_dist{100.0, 150.0}; ///< Generates a random massive values between 100.0 and 150.0 to fake an anomaly.
    
	std::uniform_int_distribution<int> respondent_dist{1, 100}; ///< Randomly selects a user ID between 1 and 100 to simulate multi-tenant traffic.

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