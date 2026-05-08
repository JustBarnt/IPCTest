cmake_minimum_required(VERSION 3.30)
project(IPCTest)

set(CMAKE_CXX_STANDARD 20)


add_executable(IPCTest)
target_sources(IPCTest
  PRIVATE
  src/main.cpp
)

target_link_libraries(IPCTest PRIVATE ws2_32)

target_include_directories(IPCTest
  PRIVATE include              # My Headers
  SYSTEM PRIVATE $ENV{INCLUDE} # Windows SDK headers
)
