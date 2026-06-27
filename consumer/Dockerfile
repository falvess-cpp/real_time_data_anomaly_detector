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

# Argumentos de build apontando para o seu repositório oficial
ARG REPO_URL="https://github.com/falvess-cpp/real_time_data_anomaly_detector.git"
ARG REPO_VERSION="main"

# Clona o ecossistema completo do repositório remoto
RUN git clone --depth 1 --branch ${REPO_VERSION} ${REPO_URL} .

# Navega até o subdiretório do Consumer para realizar a compilação
WORKDIR /app/consumer/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# =========================================================
# Estágio 2: Runtime - Imagem limpa e otimizada para execução
# =========================================================
FROM debian:12-slim

RUN apt-get update && apt-get install -y \
    libhiredis1.1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /root/

# Extrai o binário final do Consumer limpo de dependências de build
COPY --from=builder /app/consumer/build/consumer_app .

ENV REDIS_HOST=127.0.0.1
ENV REDIS_PORT=6380

CMD ["./consumer_app"]