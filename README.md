# Real-Time Data Anomaly Detector

A high-performance, low-latency multi-tenant stream processing pipeline built in modern C++ (C++20). The system ingests continuous numerical metric streams via Redis Pub/Sub, routes traffic deterministically to mitigate multi-threaded lock contention, and evaluates data points in real time against a rolling historical baseline using a sliding Z-score statistical model.

## 🏗️ Architecture Overview

The system is split into two specialized components coordinating over a lightweight messaging topology:

1. **Producer Service (Data Generator):** Simulates multi-tenant transaction workloads. It models standard respondent behavior using a normal distribution ($\mu = 50.0, \sigma = 5.0$) while intentionally injecting controlled 5% Bernoulli outliers ($[100.0, 150.0]$) representing anomalous behavior.
2. **Consumer Service (Anomaly Detector):** A highly parallelized multi-threaded engine optimized for zero-allocation performance on the hot path.

### Core Engineering Design Choices:
* **Zero-Allocation Memory Layer (`ObjectPool`):** Eliminates constant calls to the global OS heap allocator (`new`/`delete`) during continuous streaming. Objects are pre-allocated at boot and recycled, preventing heap fragmentation and non-deterministic latency spikes (jitter).
* **Consistent Hashing / Sticky Routing (`Dispatcher`):** Computes a deterministic hash on the incoming `respondent_id`. This forces all metrics belonging to a specific tenant into the exact same thread-local queue worker. This guarantees chronological execution order per tenant and keeps the worker's internal registries completely lock-free.
* **Un-poisoned Baseline (`RollingWindow`):** Evaluates incoming signals against the existing sliding historical queue ($N = 50$ to $100$) *before* updating the window. This prevents large anomalies from inflating the standard deviation and "masking" their own Z-score transformation.

---

## 🚀 Getting Started

### Prerequisites
* Docker and Docker Compose installed on your host system.

### Running the Complete Ecossystem
From the absolute root directory of the project, execute the following commands:

```bash
# 1. Build the multi-stage optimized C++ images
docker-compose build

# 2. Run the full environment (Redis, Producer, and Consumer) in background
docker-compose up -d

# 3. Stream the Consumer logs to monitor real-time anomaly flagging
docker logs -f c_realtime_consumer
```

---

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
│   │   └── producer.hpp     # Class definitions & hyper-parameters
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