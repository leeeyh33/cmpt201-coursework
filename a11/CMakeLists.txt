cmake_minimum_required(VERSION 3.10)

project(a11 C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(server src/server.c)
add_executable(client src/client.c)

target_link_libraries(server pthread)
target_link_libraries(client pthread)
