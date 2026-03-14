cmake_minimum_required(VERSION 3.16)

project(mapreduce C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(mapreduce
    src/main.c
    src/test.c
    src/single_map.c
    src/single_reduce.c
    src/map_and_reduce.c
    src/number_of_mappers_reducers.c
    src/partition.c
    src/free_output.c
    src/mapreduce.c
)

target_link_libraries(mapreduce pthread)
