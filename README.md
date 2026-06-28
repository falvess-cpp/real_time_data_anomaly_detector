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

Core Engineering Design Choices:

* Bounded Zero-Allocation Memory Layer (ObjectPool): 
Eliminates constant calls to the global OS heap allocator (new/delete) during continuous streaming. Objects are pre-allocated at boot and recycled. To handle high-traffic bursts without introducing latency jitter, the pool features a deterministic capacity cap, safely deallocating transient surplus objects outside critical execution paths.

* Thread-Confined State Routing (Dispatcher): 
Computes a deterministic modular hash on the incoming respondent_id (Sticky Routing). This forces all metrics belonging to a specific tenant into the exact same background worker queue, ensuring absolute chronological processing and allowing the worker's internal registries to run 100% lock-free.

* Un-poisoned Baseline (RollingWindow): 
Evaluates incoming signals against the sliding historical queue ($N = 50$ to $100$) before updating the window. This protects against floating-point drift and prevents large consecutive anomalies from inflating the standard deviation, preserving the sensitivity of the Z-score transformation.

---

## 🚀 Getting Started

Prerequisites:
Docker and Docker Compose installed on your host system.

Running the Complete Ecosystem
From the absolute root directory of the project, execute the following commands:

```bash
# 1. Build the multi-stage optimized C++ images
docker-compose build

# 2. Run the full environment (Redis, Producer, and Consumer) in the background
docker-compose up -d

# 3. Stream the Consumer logs to monitor real-time anomaly flagging
docker logs -f c_realtime_consumer
```

Expected Output Log Output
When monitoring the consumer logs, you will observe the pipeline actively partitioning multi-tenant workloads across separate internal OS threads:

[Thread: 14023948293] [1719597100] Data point: 49.20 | Status: NORMAL   | Z-score: 0.15 | [ID: resp_12]
[Thread: 14023948293] [1719597101] Data point: 52.10 | Status: NORMAL   | Z-score: 0.42 | [ID: resp_12]
[Thread: 14023948500] [1719597102] Data point: 142.50 | Status: ANOMALY DETECTED! | Z-score: 18.40 | ALERT: Significant deviation detected. [ID: resp_45]

##🔮 Up Next & Production Roadmaps

1. Code Maintenance & Tooling Ecosystem

To evolve this project into an enterprise-grade repository maintained by a distributed engineering team, the following tooling additions are highly recommended:

Testing Framework: 
Integration of GoogleTest (GTest) for deterministic unit testing of the stateful RollingWindow math, alongside GoogleMock to isolate components by mocking the network behavior of the sw::redis client.

Static Analysis & Linters: 
Setup of clang-format to enforce an automated, unified coding standard across CI pipelines, and clang-tidy as a compiler frontend linter to catch modernization opportunities, unintended conversions, or potential memory management pitfalls.

Package Management: 
Migrate raw manual tracking of third-party libraries (like nlohmann/json and redis++) to vcpkg or Conan, integrating them directly into the CMakeLists.txt toolchain specification for predictable cross-platform compilation.

CI/CD Automation: 
A GitHub Actions workflow executing multi-stage Docker builds, triggering static analysis checks, and running the test suite on every pull request before target branch merges.

2. Orchestration & Kubernetes Deployment

This pipeline is fully architected to be cloud-native, making it completely eligible for deployment onto orchestration platforms like Kubernetes (EKS, GKE, or native clusters). To bridge the current setup to K8s production environments, the following elements must be implemented:

Deployment Manifests / Helm Charts: 
Creation of standard Helm charts defining Deployment objects for the Consumer and Producer services, alongside a managed Redis instance (or a stateful container orchestration pattern using an operator like Redis-Operator).

Workload Scaling Model Change (Crucial Nuance): 
The current system uses standard Redis Pub/Sub, which operates on a broadcast pattern (fan-out). If you scale the Consumer to multiple Kubernetes pods under this model, every pod will receive every message, breaking our thread-confined data architecture and multiplying CPU usage redundantly. To scale horizontally in K8s, the messaging backend must transition to Redis Streams (with Consumer Groups) or Apache Kafka. This guarantees that workloads are partitioned so that each pod owns a distinct, mutually exclusive subset of respondent_ids.

Health & Lifecycle Probes: 
Implement an embedded HTTP server (using an ultra-lightweight library like crow or a basic socket listener) inside main.cpp exposing /healthz (Liveness) and /readyz (Readiness) endpoints. This allows Kubernetes to continuously monitor if the internal Redis subscription connection has stalled or dropped, triggering automated self-healing container recycles.

Observability Ingestion: 
Instrument the Hot Path using a telemetry library like prometheus-cpp to expose core performance counters (messages processed/sec, object pool utilization rate, active memory window sizes, and anomaly counts). These metrics would be scraped by a Prometheus Operator and rendered on Grafana dashboard towers.

3. Missing Technical Requirements & Architectural Thoughts
While highly optimized for performance, a production-grade deployment would require addressing the following real-world constraints:

State Durability & Persistence: 
* The Problem: The sliding baseline states (local_windows) currently reside entirely inside volatile worker RAM. If a Consumer pod crashes or restarts due to an infrastructure event, the historical baseline for all tenants is wiped out. New metrics will be blind until the windows collect enough entries (min_size = 50) to compute Z-scores again.

The Mitigation: 
Implement a periodic state-snapshotting worker. This background routine would serialize the historical metrics deques into high-speed persistent keys (such as Redis Hashes or a TimeSeries backend) allowing pods to rebuild their local state instantly upon container boot.

Dynamic Configuration Hot-Reloading:
The Problem: Hyper-parameters like the Z-score threshold (3.0), minimum window sizes (50), and the Redis target endpoints are bound at compilation or boot time.
The Mitigation: Integrate a dynamic configuration listener thread. By subscribing to a dedicated Redis configuration channel (config_stream), operators can publish runtime tuning payloads (e.g., changing a sensitive threshold to 3.5 on the fly during a massive traffic event) without triggering a service redeployment.

Graceful Signal Handling:

The Problem: Terminating the container via standard docker kill interrupts can leave un-acked or half-processed payloads stranded in the worker's thread queues.

The Mitigation: Bind custom UNIX signal handlers (SIGINT/SIGTERM) in main.cpp. Upon interception, the application should cleanly halt the Redis subscription consumption loop, allow active background threads to flush their pending queue workloads back to the ObjectPool, log final pipeline statistics, and exit with a clean 0 code.

## 🛠️ Project Structure

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
		
		