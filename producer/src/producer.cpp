/**
 * @file producer.cpp
 * @brief Implementation details for the streaming Producer class.
 *
 * Implements the constructor network routing and the core event execution loop
 * for publishing simulated records to the pipeline network.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#include "producer.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Producer::Producer(std::string redis_host, std::string redis_port)
    : host(std::move(redis_host)),
      port(std::move(redis_port))
{
    // Constructor body remains empty as all statistical engines are 
    // cleanly initialized within the class header declaration frame.
}

void Producer::run() {
    std::cout << "[PRODUCER] Starting Redis Connection at " << host << ":" << port << "\n";
    
    // Context instantiation for the Redis client infrastructure
    auto redis = sw::redis::Redis("tcp://" + host + ":" + port);
    
    std::cout << "[PRODUCER] Streaming Pipeline active. Sending messages...\n";

    while (true) {
        double metric_value = 0.0;
        
        // Evaluate state parameters to decide whether to emit normal telemetry or outliers
        if (anomaly_trigger(gen)) {
            metric_value = anomaly_dist(gen);
        } else {
            metric_value = normal_dist(gen);
        }

        // Standardize numeric data points to exactly two decimal metrics
        metric_value = std::round(metric_value * 100.0) / 100.0;

        int id_suffix = respondent_dist(gen);
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        // Structure multi-tenant event schemas
        json payload = {
            {"respondent_id", "resp_" + std::to_string(id_suffix)},
            {"metric_value", metric_value},
            {"timestamp", timestamp}
        };

        // Broadcast message frame through target channel
        redis.publish("cint_events", payload.dump());

        // Throttling step to enforce target execution bandwidth bounds (100ms interval)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}