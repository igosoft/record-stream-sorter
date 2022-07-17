cmake_minimum_required(VERSION 3.0.0)
project(record_stream VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

 
include_directories(src)
add_executable(record_stream record_stream.cpp )
target_link_libraries(record_stream)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
