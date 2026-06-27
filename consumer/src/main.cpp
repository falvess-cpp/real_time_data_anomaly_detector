/**
 * @file main.cpp
 * @brief Runtime driver coordinating network ingestion and asynchronous analytics splits.
 *
 * Spawns tracking infrastructure components, establishes connection context subscriptions,
 * parses input message frames, and pumps pipeline layers.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#include "object_pool.h"
#include "dispatcher.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <memory>
#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Orchestration layer pointers initialized inside scope boots
std::unique_ptr<ObjectPool> g_object_pool;
std::unique_ptr<Dispatcher> g_dispatcher;

/**
 * @brief Main bootstrap driver routine.
 * @return int 0 on graceful execution hooks, 1 on unhandled connection or system failures.
 */
int main() {
    // Extract network binding targets from environment contexts
    const char* env_host = std::getenv("REDIS_HOST");
    const char* env_port = std::getenv("REDIS_PORT");
    
    std::string host = env_host ? env_host : "127.0.0.1";
    std::string port = env_port ? env_port : "6380"; // Port 6380 targeted to route past local port conflicts

    std::cout << "[CONSUMER] Initializing Real-Time Data Anomaly Detector Framework...\n";

    // Initialize the centralized pre-allocated memory recycled structure stack
    g_object_pool = std::make_unique<ObjectPool>(2000);

    // Compute hardware capabilities to maximize system parallelism bounds
    size_t concurrent_lanes = std::thread::hardware_concurrency();
    if (concurrent_lanes == 0) concurrent_lanes = 4;

    // Spawn the routing tier with N=50 window sizes allocated per respondent tracking instance
    g_dispatcher = std::make_unique<Dispatcher>(*g_object_pool, concurrent_lanes, 50, 100);

    try 
	{
		auto redis = sw::redis::Redis("tcp://" + host + ":" + port);
		auto subscriber = redis.subscriber();

        // Assign asynchronous transaction handler callbacks to incoming Pub/Sub streaming strings
        subscriber.on_message([](std::string /*channel*/, std::string message_frame) {
		try {
                // Parse message payload
                auto parsed_payload = json::parse(message_frame);

                // Acquire memory slice tracking framework from the ObjectPool
                DataPoint* memory_slice = g_object_pool->acquire();

                // Populate context properties using non-allocating types
                memory_slice->respondent_id = parsed_payload["respondent_id"].get<std::string>();
                memory_slice->metric_value = parsed_payload["metric_value"].get<double>();
                memory_slice->timestamp = parsed_payload["timestamp"].get<long long>();

                // Shift ownership downstream into parallel analytical workers
                g_dispatcher->route(memory_slice);

            } catch (const std::exception& error) {
                std::cerr << "[WARNING] Discarding invalid payload string structure framing: " << error.what() << "\n";
            }
        });

        // Open structural bindings onto channels and enter loop tracking executions
        subscriber.subscribe("cint_events");
        std::cout << "[CONSUMER] Stream connection established. Listening to 'cint_events' channel...\n";

        while (true) {
            subscriber.consume(); // Blocking I/O socket pooling execution block waiting on incoming network signals
        }

    } catch (const sw::redis::Error& error) {
        std::cerr << "[CRITICAL ERROR] Messaging infrastructure connection dropped down: " << error.what() << "\n";
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "[CRITICAL ERROR] Execution aborted: " << error.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "[CRITICAL ERROR] Execution interrupted by unmapped panic bubble\n";
        return 1;
    }

    return 0;
}