cmake_minimum_required(VERSION 3.15)
project(cint_producer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -Wall -Wextra")

include(FetchContent)

# nlohmann_json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# Disable Redis++ internal tests
set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(REDIS_PLUS_PLUS_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# redis-plus-plus
FetchContent_Declare(
    redisplusplus
    GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
    GIT_TAG 1.3.10
)
FetchContent_MakeAvailable(redisplusplus)

# hiredis
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

add_executable(producer_app 
    src/main.cpp 
    src/producer.cpp
)

target_link_libraries(producer_app 
    PRIVATE 
        nlohmann_json::nlohmann_json
        redis++
        ${HIREDIS_LIBRARIES}
)

target_include_directories(producer_app 
    PRIVATE 
        include 
        ${HIREDIS_INCLUDE_DIRS}
)