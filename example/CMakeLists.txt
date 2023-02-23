cmake_minimum_required(VERSION 3.22)
project(fsm_example)

add_executable(${PROJECT_NAME} OrderFSM.cpp OrderFSM.hpp)
target_link_libraries(${PROJECT_NAME} fsm::fsm)