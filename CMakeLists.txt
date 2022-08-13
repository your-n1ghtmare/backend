cmake_minimum_required(VERSION 3.10.2)
project(Backend)

set(CMAKE_CXX_STANDARD 11)

add_executable(Backend main.cpp Server.cpp Server.hpp)
target_link_libraries(Backend mysqlcppconn)