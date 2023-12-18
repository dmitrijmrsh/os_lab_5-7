cmake_minimum_required(VERSION 3.20)
project(lab5-7)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-lzmq)

add_executable(control ControlNode.cpp)
add_executable(calculation CalculationNode.cpp)

target_link_libraries(control zmq)
target_link_libraries(calculation zmq)