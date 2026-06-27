/**
 * @file main.cpp
 * @brief Application execution driver acting as the bootstrapper context for the workflow.
 *
 * Resolves context environments for host routing, provisions the Producer instance 
 * on stack context, and catches bubble exceptions thrown out of runtime processes.
 *
 * @author Fabiano Souza
 * @date 2026
 */

#include "producer.hpp"
#include <iostream>
#include <cstdlib>
#include <sw/redis++/redis++.h>

/**
 * @brief Main execution entry point.
 * @return int 0 on graceful execution completion hooks, 1 on critical context termination failures.
 */
int main() {
    // Resolve environment targets from system bindings with default engine test configuration fallbacks
    const char* env_host = std::getenv("REDIS_HOST");
    const char* env_port = std::getenv("REDIS_PORT");
    
    std::string host = env_host ? env_host : "127.0.0.1";
    std::string port = env_port ? env_port : "6380";

    try {
        // Construct and execute the processing engine instance
        Producer producer(host, port);
        producer.run();
    } 
    catch (const sw::redis::Error& e) {
        std::cerr << "[ERROR] Communication Critical Exception on Producer: " << e.what() << "\n";
        return 1;
    } 
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << "\n";
        return 1;
    } 
    catch (...) {
        std::cerr << "[ERROR] Unknown Exception caught at global driver boundary\n";
        return 1;
    }    

    return 0;
}