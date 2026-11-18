
cmake_minimum_required(VERSION 3.16)
project(order_book LANGUAGES CXX)

add_executable(order_book src/main.cpp)
target_include_directories(order_book PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(order_book PRIVATE cxx_std_17)
