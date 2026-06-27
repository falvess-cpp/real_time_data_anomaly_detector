# ==========================================
# Stage 1: Dynamic Build & Compilation Engine
# ==========================================
FROM debian:12-slim AS builder

# Install essential compilation toolchains and library requirements
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libhiredis-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the local source files into the builder layer context
COPY . .

# Compile optimized Release targets utilizing all hardware processing cores
WORKDIR /app/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# ==========================================
# Stage 2: Hardened Runtime Environment
# ==========================================
FROM debian:12-slim

# Install only the core runtime shared library package for hiredis
RUN apt-get update && apt-get install -y \
    libhiredis1.1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/

# Extract only the isolated static binary from the compilation context stage
COPY --from=builder /app/build/consumer_app .

# Define fallback infrastructure binding environments
ENV REDIS_HOST=127.0.0.1
ENV REDIS_PORT=6380

# Execute entry points directly on stack
CMD ["./consumer_app"]