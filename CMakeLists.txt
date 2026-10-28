cmake_minimum_required(VERSION 3.20)
project(libpiazza VERSION 0.1.0 LANGUAGES CXX)

# ----- C++ / Qt boilerplate ------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Q_NAMESPACE / Q_ENUM_NS in the protocol header require moc.
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

# Vendored JSON-RPC implementation.
add_subdirectory(libqjsonrpc)

add_library(piazza
    src/protocol.h
    src/protocol.cpp
    src/http_transport.h
    src/http_transport.cpp
    src/rpc.h
    src/rpc.cpp
)

target_include_directories(piazza PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(piazza PUBLIC qjsonrpc Qt6::Core Qt6::Network)
target_compile_features(piazza PUBLIC cxx_std_20)
