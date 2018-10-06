cmake_minimum_required(VERSION 3.12)
project(threes_cpp)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(threes_cpp
        Action.h
        Agent.h
        Board64.h
        Episode.h
        Statistic.h
        LookUpTable.h
        Threes.cpp Common.h)
