cmake_minimum_required(VERSION 2.8)
project(MyTcpServer)

set(CMAKE_CXX_STANDARD 20)

add_library(session.hpp session.cpp)
add_library(server.hpp  server.cpp)
add_executable(server main.cpp)

target_link_libraries(server server.hpp)
target_link_libraries(server.hpp session.hpp)