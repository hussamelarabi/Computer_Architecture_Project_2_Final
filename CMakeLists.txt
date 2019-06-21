cmake_minimum_required(VERSION 3.14)
project(CA_Project_2_Final)

set(CMAKE_CXX_STANDARD 17)

add_executable(CA_Project_2_Final main.cpp Includes.h prediction.hpp StageLog.hpp ReorderBuffer.hpp Instruction.hpp reservationStation.hpp)