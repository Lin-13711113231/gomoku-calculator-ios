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
    eval/eval.cpp eval/evalconfig.cpp eval/evaluator.cpp
    eval/mix9svqnnue.cpp eval/mix10nnue.cpp
    game/board.cpp game/movegen.cpp game/pattern.cpp
    search/hashtable.cpp search/movepick.cpp search/opening.cpp
    search/searchcommon.cpp search/searchconfig.cpp search/searchengine.cpp
    search/searchoutput.cpp search/searchthread.cpp search/timecontrol.cpp
    search/ab/history.cpp search/ab/search.cpp
    search/mcts/node.cpp search/mcts/search.cpp
    config.cpp internalConfig.cpp)

# This project lives in native/, while the sources above live in the separately
# checked-out Rapfi tree. Convert the list before passing it to add_library so
# CMake does not look for e.g. native/command/argutils.cpp.
list(TRANSFORM RAPFI_SOURCES PREPEND "${RAPFI_SOURCE_DIR}/")

add_subdirectory(${RAPFI_SOURCE_DIR}/external/cpptoml ${CMAKE_BINARY_DIR}/cpptoml)
add_subdirectory(${RAPFI_SOURCE_DIR}/external/cxxopts ${CMAKE_BINARY_DIR}/cxxopts)
add_subdirectory(${RAPFI_SOURCE_DIR}/external/lz4 ${CMAKE_BINARY_DIR}/lz4)
add_subdirectory(${RAPFI_SOURCE_DIR}/external/simde ${CMAKE_BINARY_DIR}/simde)

add_library(rapfi_native STATIC ${RAPFI_SOURCES} RapfiEngineBridge.cpp)
target_include_directories(rapfi_native PRIVATE ${RAPFI_SOURCE_DIR})
target_compile_definitions(rapfi_native PRIVATE MULTI_THREADING NO_COMMAND_MODULES)
target_link_libraries(rapfi_native PRIVATE cpptoml cxxopts lz4 simde)

if(APPLE)
    target_link_libraries(rapfi_native PRIVATE pthread)
endif()

# iPad/iPhone builds are arm64. Enable the baseline ARM SIMD path; dot-product
# is enabled by the workflow when the AppleClang toolchain supports it.
option(USE_NEON "Enable ARM NEON" ON)
option(USE_NEON_DOTPROD "Enable ARM NEON dot product" OFF)
if(USE_NEON_DOTPROD)
    target_compile_definitions(rapfi_native PRIVATE USE_NEON USE_NEON_DOTPROD)
    target_compile_options(rapfi_native PRIVATE -march=armv8.2-a+dotprod)
elseif(USE_NEON)
    target_compile_definitions(rapfi_native PRIVATE USE_NEON)
    target_compile_options(rapfi_native PRIVATE -march=armv8-a+simd)
endif()

add_library(rapfi_plugin STATIC RapfiPlugin.m)
target_include_directories(rapfi_plugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(CAPACITOR_INCLUDE_DIR)
    target_include_directories(rapfi_plugin PRIVATE "${CAPACITOR_INCLUDE_DIR}")
endif()
if(CAPACITOR_HEADER)
    get_filename_component(CAPACITOR_MODULE_DIR "${CAPACITOR_HEADER}" DIRECTORY)
    get_filename_component(CAPACITOR_PACKAGE_DIR "${CAPACITOR_MODULE_DIR}" DIRECTORY)
    target_include_directories(rapfi_plugin PRIVATE
        "${CAPACITOR_MODULE_DIR}"
        "${CAPACITOR_PACKAGE_DIR}")
    target_compile_definitions(rapfi_plugin PRIVATE
        CAPACITOR_HEADER_PATH=\"${CAPACITOR_HEADER}\")
endif()
