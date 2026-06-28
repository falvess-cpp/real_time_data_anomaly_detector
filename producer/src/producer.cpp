/**
 * @file producer.cpp
 * @brief Implementation of the real-time data streaming Producer.
 *
 * Handles the initialization of the Redis network connectivity and manages the 
 * core continuous ingestion loop. It generates high-frequency synthetic telemetry 
 * data—including controlled anomaly injection—and serializes the multi-tenant 
 * payloads into a shared messaging backbone.
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

}

void Producer::run() {
    std::cout << "[PRODUCER] Starting Redis Connection at " << host << ":" << port << "\n";
    
    auto redis = sw::redis::Redis("tcp://" + host + ":" + port);
    
    std::cout << "[PRODUCER] Streaming Pipeline active. Sending messages...\n";

    while (true) {
        double metric_value = 0.0;
        
		bool inject_anomaly = anomaly_trigger(gen);

		if (inject_anomaly) {
			metric_value = anomaly_dist(gen);
		} else {
			metric_value = normal_dist(gen);
		}

        metric_value = std::round(metric_value * 100.0) / 100.0;

        int id_suffix = respondent_dist(gen);
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        json payload = {
            {"respondent_id", "resp_" + std::to_string(id_suffix)},
            {"metric_value", metric_value},
            {"timestamp", timestamp}
        };

        redis.publish("metrics_stream", payload.dump());

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}