cmake_minimum_required(VERSION 4.0)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)
add_library(executorlib STATIC ExecutorApp.cpp)
target_include_directories(executorlib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${QUICKFIX_INCLUDE_DIR})
target_link_libraries(executorlib quickfix pthread)
