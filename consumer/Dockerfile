# =========================================================
# Estágio 1: Builder - Clona do GitHub e compila em Release
# =========================================================
FROM debian:12-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libhiredis-dev \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

ARG REPO_URL="https://github.com/falvess-cpp/real_time_data_anomaly_detector.git"
ARG REPO_VERSION="master"

RUN git clone --depth 1 --branch ${REPO_VERSION} ${REPO_URL} .

WORKDIR /app/consumer/build
# Compila e centraliza os artefatos (.so e binário) em uma pasta /app/dist
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc) && \
    mkdir -p /app/dist && \
    cp consumer_app /app/dist/ && \
    find . -name "libredis++.so*" -exec cp -d {} /app/dist/ \;

# =========================================================
# Estágio 2: Runtime - Imagem limpa e otimizada para execução
# =========================================================
FROM debian:12-slim

RUN apt-get update && apt-get install -y \
    libhiredis0.14 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/

# Copia o binário executável
COPY --from=builder /app/dist/consumer_app .

# Copia as bibliotecas dinâmicas do redis++ e atualiza o cache do linker do SO
COPY --from=builder /app/dist/libredis++.so* /usr/local/lib/
RUN ldconfig

ENV REDIS_HOST=127.0.0.1
ENV REDIS_PORT=6380

CMD ["./consumer_app"]