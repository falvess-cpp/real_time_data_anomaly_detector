cmake_minimum_required(VERSION 3.15)
project(cint_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -Wall -Wextra -pthread")

include(FetchContent)

# Dependency: nlohmann_json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Disable internal redis++ unit tests compilation
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Dependency: redis-plus-plus
FetchContent_Declare(
    redisplusplus
    GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
    GIT_TAG 1.3.10
)
FetchContent_MakeAvailable(redisplusplus)

# System Dependency: hiredis
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

# Declare target executable with all source components
add_executable(consumer_app 
    src/main.cpp 
    src/dispatcher.cpp 
    src/worker.cpp 
    src/object_pool.cpp
)

target_link_libraries(consumer_app 
    PRIVATE 
        nlohmann_json::nlohmann_json
        redis++
        ${HIREDIS_LIBRARIES}
        pthread
)

# Bind include paths to the target application framework
target_include_directories(consumer_app 
    PRIVATE 
        include 
        ${HIREDIS_INCLUDE_DIRS}
)