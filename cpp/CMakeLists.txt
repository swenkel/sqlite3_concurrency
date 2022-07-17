cmake_minimum_required (VERSION 3.18)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-O3")
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_CPPCHECK "cppcheck")

project(sqlite_concurrency_benchmark)

find_package(SQLite3)
find_package(Threads)

include_directories(
	${SQLite3_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(
	sqlite_concurrency_benchmark_cpp
	sqlite_concurrency_benchmark_cpp.cc
)

target_link_libraries (
	sqlite_concurrency_benchmark_cpp
	${SQLite3_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)
