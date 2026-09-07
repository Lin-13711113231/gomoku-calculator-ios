cmake_minimum_required(VERSION 3.18)
project(gomoku_rapfi_native LANGUAGES C CXX OBJC)

if(NOT RAPFI_SOURCE_DIR)
    message(FATAL_ERROR "RAPFI_SOURCE_DIR must point to a Rapfi checkout")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RAPFI_SOURCES
    command/argutils.cpp command/benchmark.cpp command/command.cpp
    command/dbcommand.cpp command/gomocup.cpp
    core/compressor.cpp core/hash.cpp core/iohelper.cpp core/utils.cpp
    core/platform.cpp core/version.cpp
    database/dbclient.cpp database/dbconfig.cpp database/dbutils.cpp
    database/dbtypes.cpp database/renlib.cpp database/yxdbstorage.cpp
    eval/eval.cpp internalConfig.cpp)

add_subdirectory(${RAPFI_SOURCE_DIR}/external/cpptoml ${CMAKE_BINARY_DIR}/cpptoml)
add_subdirectory(${RAPFI_SOURCE_DIR}/external/cxxopts ${CMAKE_BINARY_DIR}/cxxopts)
add_subdirectory(${RAPFI_SOURCE_DIR}/external/lz4 ${CMAKE_BINARY_DIR}/lz4)
add_subdirectory(${RAPFI_SOURCE_DIR}/external/simde ${CMAKE_BINARY_DIR}/simde)

add_library(rapfi_native STATIC ${RAPFI_SOURCES} RapfiEngineBridge.cpp)
target_include_directories(rapfi_native PRIVATE ${RAPFI_SOURCE_DIR})
target_compile_definitions(rapfi_native PRIVATE MULTI_THREADING NO_COMMAND_MODULES)
target_link_libraries(rapfi_native PRIVATE cpptoml cxxopts lz4 simde)

add_library(rapfi_plugin STATIC RapfiPlugin.m)
target_include_directories(rapfi_plugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(CAPACITOR_INCLUDE_DIR)
    target_include_directories(rapfi_plugin PRIVATE ${CAPACITOR_INCLUDE_DIR})
endif()
