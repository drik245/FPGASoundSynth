cmake_minimum_required(VERSION 3.20)
project(minilogue_synth VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Audio sample rate and bit depth
add_compile_definitions(
    SAMPLE_RATE=192000
    BIT_DEPTH=24
    NUM_VOICES=4
)

# Source files
set(SOURCES
    src/main.cpp
)

# Header-only library structure
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/effects
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/include
)

# Windows-specific audio backends
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ole32 winmm)
endif()

# Linux-specific
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread m dl)
endif()

# macOS-specific
if(APPLE)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COREAUDIO_LIBRARY}
        ${AUDIOUNIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
    )
endif()
