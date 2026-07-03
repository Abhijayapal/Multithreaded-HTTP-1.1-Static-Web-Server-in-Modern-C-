cmake_minimum_required(VERSION 3.10)
project(HttpServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17) # Require C++17 to get modern features like string_view and filesystem
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add include directory
include_directories(include)

# Compile the source files
set(SOURCES
    src/main.cpp
    src/Logger.cpp
    src/SocketServer.cpp
    src/HttpRequestParser.cpp
    src/FileService.cpp
    src/ThreadPool.cpp
    src/LRUCache.cpp
)

# compile the files of sources into final runnable program named server
add_executable(server ${SOURCES})

# Add pthread for threading (required on Linux/POSIX)
find_package(Threads REQUIRED)
target_link_libraries(server Threads::Threads)
