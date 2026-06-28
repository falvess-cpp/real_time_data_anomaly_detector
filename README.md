# Real-Time Data Anomaly Detector

A high-performance, low-latency multi-tenant stream processing pipeline built in modern C++ (C++20). The system ingests continuous numerical metric streams via Redis Pub/Sub, routes traffic deterministically to mitigate multi-threaded lock contention, and evaluates data points in real time against a rolling historical baseline using an in-memory sliding Z-score statistical model.

## 🏗️ Architecture Overview

The system is split into two specialized components coordinating over a lightweight messaging topology:

1. **Producer Service (Data Generator):** Simulates concurrent multi-tenant transaction workloads. It models standard respondent behavior using a normal distribution ($\mu = 50.0, \sigma = 5.0$) while intentionally injecting controlled 5% Bernoulli outliers ($[100.0, 150.0]$) representing anomalous behavior.
2. **Consumer Service (Anomaly Detector):** A highly parallelized multi-threaded engine optimized for zero-allocation performance on the hot path.

### Pipeline Architecture Map (Many-to-One Topology)

```text
  +-----------------+             +-----------------+
  |   Producer 1    |             |   Producer N    |  <- Multiple instances generating
  | (Normal/Bernou) |             | (Normal/Bernou) |     asynchronous metrics data
  +-----------------+             +-----------------+
           |                               |
           +---------------+---------------+
                           |  Publishes JSON Payloads
                           v
              +-------------------------+
              |   Redis Pub/Sub Broker  |  <- Centralized message broker
              |    ("metrics_stream")   |     (Topology: Many-to-One)
              +-------------------------+
                           |
                           v  subscriber.consume() (Single-Thread Path)
              +-------------------------+
              |  Consumer: Main Thread  |  <- Parses JSON and acquires raw
              |  (JSON Parse & Acquire) |     pointers from the ObjectPool
              +-------------------------+
                           |
                           v  g_dispatcher->route(point)
              +-------------------------+
              |  Dispatcher (Routing)   |  <- Computes Hash(respondent_id) % PoolSize
              +-------------------------+
                 /          |          \
                /           |           \   Sticky Routing (Guarantees chronological order)
               v            v            v
        +------------+ +------------+ +------------+
        |  Worker 1  | |  Worker 2  | |  Worker N  |  <- Active OS Thread Pool
        | (Queue t1) | | (Queue t2) | | (Queue tN) |     (jthread + condition_variable)
        +------------+ +------------+ +------------+
              |              |              |
              v              v              v
        +------------+ +------------+ +------------+
        | Local Map  | | Local Map  | | Local Map  |  <- Thread-Confined State Store
        |  Rolling   | |  Rolling   | |  Rolling   |     (Isolated memory windows per tenant,
        |  Windows   | |  Windows   | |  Windows   |      running 100% LOCK-FREE)
        +------------+ +------------+ +------------+
```

Core Engineering Design Choices:

* RollingWindow: 
Evaluates incoming signals against the sliding historical queue ($N = 50$ to $100$) before updating the window. This protects against floating-point drift and prevents large consecutive anomalies from inflating the standard deviation, preserving the sensitivity of the Z-score transformation.

* ObjectPool: 
Eliminates constant calls to the global OS heap allocator (new/delete) during continuous streaming. Objects are pre-allocated at boot and recycled. To handle high-traffic bursts without introducing latency jitter, the pool features a deterministic capacity cap, safely deallocating transient surplus objects outside critical execution paths.

* Dispatcher: 
Computes a deterministic modular hash on the incoming respondent_id (Sticky Routing). This forces all metrics belonging to a specific tenant into the exact same background worker queue, ensuring absolute chronological processing and allowing the worker's internal registries to run 100% lock-free.

## 🚀 Getting Started

### Prerequisites
* **For Containerized Execution:** Docker and Docker Compose installed.
* **For Local Native Compilation:** CMake (>= 3.15), a C++20 compliant compiler (GCC >= 10 or Clang >= 11), and local development libraries for `redis++` and `nlohmann/json`.
---

### Method 1: Running with Docker Compose (Recommended)
From the absolute root directory of the project, execute the following commands to orchestrate the entire eco-system:

```bash
# 1. Build the multi-stage optimized C++ images
docker-compose build

# 2. Run the full environment (Redis, Producer, and Consumer) in the background
docker-compose up -d

# 3. Stream the Consumer logs to monitor real-time anomaly flagging
docker logs -f c_realtime_consumer
```
---

### Method 2: Building and Running Locally (Native Compilation)
If you prefer to compile and execute the binaries directly on your host machine (outside Docker), follow these steps.

Note: Ensure your Redis server is up and running on port 6380 beforehand.

1. Compile and Run the Producer Service:

```bash
cd ../../producer
mkdir build && cd build
cmake ..
make
./producer_app
```

2. Compile and Run the Consumer Service:

```bash
cd consumer
mkdir build && cd build
cmake ..
make
./consumer_app
```

3. Monitoring the Live Redis Telemetry Stream

```bash
# If running via Docker Compose:
docker exec -it c_redis_messaging redis-cli -p 6380 MONITOR

# If running against a local host Redis server:
redis-cli -p 6380 MONITOR
```
---

### Expected Output Log Output
When monitoring the consumer logs, you will observe the pipeline actively partitioning multi-tenant workloads across separate internal OS threads:
```bash
[Thread: 127236206339776][1782687150] Data point: 51.69 | Status: OK | Z-score: 0.19 | [ID: resp_100]
[Thread: 127236164376256][1782687150] Data point: 55.10 | Status: OK | Z-score: 0.21 | [ID: resp_63]
[Thread: 127236223125184][1782687150] Data point: 115.31 | Status: ANOMALY DETECTED! | Z-score: 4.70 | ALERT: Significant deviation detected. [ID: resp_61]
[Thread: 127236164376256][1782687152] Data point: 52.91 | Status: OK | Z-score: 0.18 | [ID: resp_90]
```
---

## 🔮 Up Next

### 1. Code Maintenance & Tooling Ecosystem

To evolve this project into an enterprise-grade repository maintained by distributed engineering teams, the following tooling stack is recommended for integration:

* **Testing Frameworks & E2E Regression:** Integration of **GoogleTest (GTest)** for deterministic unit testing of the stateful `RollingWindow` math, alongside **GoogleMock** to isolate network behaviors. Furthermore, GTest can orchestrate full **Integration Testing** pipelines by spinning up a *Fake Producer*. This component injects a pre-defined, deterministic historical dataset into the Redis stream, allowing the system to capture the Consumer's live outputs (Z-scores and anomaly logs) and validate them against a strict testing baseline ("golden master") to prevent mathematical or logical regressions across builds.

* **Static Analysis & Linters:** Setup of **`clang-format`** to enforce an automated, unified coding standard (e.g., LLVM or Google style) via pre-commit hooks, and **`clang-tidy`** as a compiler frontend linter to catch modernization opportunities, unintended type conversions, or potential concurrency pitfalls.

* **Modern Package Management:** Migrate manual dependency tracking of third-party libraries (like `nlohmann/json` and `redis++`) to a modern C++ package manager such as **vcpkg** or **Conan**, integrating them directly into the `CMakeLists.txt` toolchain specification for predictable cross-platform compilation.

* **CI/CD Pipeline Automation:** A continuous integration workflow (e.g., **GitHub Actions**) configured to run automatically on every Pull Request (PR) or code push. This pipeline enforces mandatory quality gates by executing multi-stage Docker builds, running static analysis linters, and triggering the entire GoogleTest regression suite (including the *Fake Producer* E2E tests).
---

### 2. Orchestration & Kubernetes Deployment
This cloud-native pipeline is fully architected to be deployed and scaled horizontally within an orchestration platform like Kubernetes. From a System Design perspective, scaling this service distributedly requires transitioning the messaging layer from standard Redis Pub/Sub to a partitioned log system like Redis Streams or Apache Kafka. This architectural shift ensures that metric workloads are consistently partitioned across multiple cluster pods by hashing the respondent_id, scaling throughput horizontally while perfectly preserving our lock-free, thread-confined local state design.
---

### 3. Critical Missing Technical Requirements & Architectural Thoughts
While highly optimized for performance, a production-grade enterprise deployment would require addressing the following real-world architectural constraints:

* **State Durability & Persistence:**
  * *The Problem:* The sliding baseline states currently reside entirely inside volatile worker RAM. If a Consumer container or node crashes due to an infrastructure event, the historical baseline for all tenants is wiped out. New metrics will be blind until the windows collect enough entries to compute statistics again, creating an operational visibility gap.
  * *The Solution:* Implement the **Event Sourcing & Embedded Durable State Store pattern**. By backing the local sliding windows with a high-performance, low-latency embedded key-value store (such as RocksDB), states are continuously and deterministically flushed to non-volatile storage with sub-millisecond overhead. Coupled with a durable commit-log messaging layer, a recovering or newly spawned container can instantly rebuild its local state up to the exact last committed checkpoint offset, guaranteeing a **Zero-Data-Loss Recovery Time Objective (RTO)**.
---  
* **Dynamic Configuration Hot-Reloading:**
  * *The Problem:* Operational hyper-parameters—such as statistical anomaly thresholds, minimum window sizes, and network topology bounds—are fixed at boot time. Tuning these parameters to adapt to changing real-world signal noise or operational shifts requires a full service restart, leading to pipeline downtime and state teardown.
  * *The Solution:* Implement a **Lock-Free Configuration Hot-Swapping Pattern**. By encapsulating runtime tuning parameters within thread-safe atomic primitives or reference pointer wrappers, the processing engine threads can evaluate thresholds with zero lock contention. A dedicated background configuration thread listens to a control channel, allowing operators to broadcast remote configuration updates that are swapped on-the-fly without causing ingestion stalls, pipeline locks, or service restarts.
---  
* **Graceful Signal Handling & Cooperative Pipeline Drain:**
  * *The Problem:* Terminating the execution container via abrupt infrastructure or operating system interrupts kills the process immediately. This leaves half-processed or un-evaluated telemetry payloads stranded in background memory queues, drops active network sockets violently, and prevents the system from generating final pipeline diagnostics.
  * *The Solution:* Implement the **Cooperative Pipeline Drain Pattern**. By intercepting termination signals from the operating system, the application breaks the main network ingestion loop and propagates a cooperative cancellation signal across all active execution lanes. Instead of aborting immediately, the engine enters a synchronized draining state: background threads are granted a grace window to finish processing all remaining events currently queued in their internal buffers and release resources cleanly before a safe, deterministic application teardown.
---

## 🛠️ Project Structure

```text
real_time_data_anomaly_detector/
├── docker-compose.yml       # Infrastructure orchestration
├── README.md                # System documentation
├── redis/                   # Customized messaging layer
│   ├── Dockerfile           # Redis container customization
│   └── redis.conf           # Redis network and memory configuration
├── producer/
│   ├── Dockerfile           # Multi-stage release container specification
│   ├── CMakeLists.txt       # Build compilation directives
│   ├── include/
│   │   └── producer.hpp     # Class definitions & architectural boundaries
│   └── src/
│       ├── producer.cpp     # Data generation & publishing implementation
│       └── main.cpp         # Infrastructure bootstrapper driver
└── consumer/
    ├── Dockerfile           # Multi-stage release container specification
    ├── CMakeLists.txt       # Build compilation directives
    ├── include/
    │   ├── object_pool.h    # Cache recycling allocations interface
    │   ├── dispatcher.h     # High-speed routing layer definitions
    │   └── worker.h         # Concurrency lanes & statistical windows
    └── src/
        ├── object_pool.cpp  # Recycling memory stack mechanics
        ├── dispatcher.cpp   # Sticky routing implementation
        ├── worker.cpp       # Thread loops & Sliding Z-Score calculations
        └── main.cpp         # App bootstrapper & Redis Pub/Sub subscriber
```	