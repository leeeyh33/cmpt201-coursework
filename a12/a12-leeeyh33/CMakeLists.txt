cmake_minimum_required(VERSION 3.10)
project(blockchain C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

add_executable(blockchain
    src/main.c
    src/blockchain.c
)

target_link_directories(blockchain PRIVATE ${CMAKE_SOURCE_DIR}/lib)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    target_link_libraries(blockchain crypto check-arm64)
else()
    target_link_libraries(blockchain crypto check-amd64)
endif()
