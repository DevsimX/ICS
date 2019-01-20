cmake_minimum_required(VERSION 3.10)
project(network_LAB)

set(CMAKE_CXX_STANDARD 14)

include_directories(client)
include_directories(server)
add_executable(server server.c server.h mylib.c mylib.h)
add_executable(client client.c client.h mylib.c mylib.h)
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)
target_link_libraries(client Threads::Threads)
