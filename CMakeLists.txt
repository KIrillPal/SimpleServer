cmake_minimum_required(VERSION 2.8)
project(MyTcpServer)

set(CMAKE_CXX_STANDARD 20)

add_library(config.hpp  src/config.cpp)
add_library(log.hpp     src/log.cpp)
add_library(request.hpp src/request.cpp)
add_library(session.hpp src/session.cpp)
add_library(server.hpp  src/server.cpp)
add_executable(server   src/main.cpp)

target_link_libraries(config.hpp log.hpp)
target_link_libraries(request.hpp config.hpp)
target_link_libraries(session.hpp config.hpp request.hpp)
target_link_libraries(server.hpp  config.hpp session.hpp)

target_link_libraries(server config.hpp server.hpp)
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ../ )
