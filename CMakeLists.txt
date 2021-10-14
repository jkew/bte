cmake_minimum_required(VERSION 3.10)

project(bte)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

# add the executable
add_executable(bte bte.cpp
		   distribution.cpp
		   loadbalance_model.cpp
		   configure.cpp
		   simulator.cpp
		   capacity_model.cpp)

target_include_directories(bte PUBLIC
			   "${PROJECT_SOURCE_DIR}/include"
                           )

