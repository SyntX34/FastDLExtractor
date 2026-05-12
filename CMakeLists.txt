cmake_minimum_required(VERSION 3.16)

project(FastDLTool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(SOURCES
    src/main.cpp
    src/FastDLDownloader.cpp
    src/ConfigManager.cpp
)

add_executable(FastDLTool ${SOURCES})

target_include_directories(FastDLTool PRIVATE
    src
    third_party
)

if(WIN32)
    target_compile_definitions(FastDLTool PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )

    if(MINGW)
        find_path(BZ2_INCLUDE_DIR
            NAMES bzlib.h
            PATHS
                "C:/msys64/mingw64/include"
                "C:/mingw64/include"
            NO_DEFAULT_PATH
        )
        
        find_library(BZ2_LIB
            NAMES libbz2.a bz2
            PATHS
                "C:/msys64/mingw64/lib"
                "C:/mingw64/lib"
            NO_DEFAULT_PATH
        )
        
        if(NOT BZ2_INCLUDE_DIR)
            message(FATAL_ERROR "bzlib.h not found! Install bzip2 with: pacman -S mingw-w64-x86_64-bzip2")
        endif()
        
        if(NOT BZ2_LIB)
            message(FATAL_ERROR "libbz2.a not found! Install bzip2 with: pacman -S mingw-w64-x86_64-bzip2")
        endif()
        
        message(STATUS "Found BZip2 headers: ${BZ2_INCLUDE_DIR}")
        message(STATUS "Found BZip2 library: ${BZ2_LIB}")
        
        target_include_directories(FastDLTool PRIVATE ${BZ2_INCLUDE_DIR})
        target_link_libraries(FastDLTool PRIVATE
            ${BZ2_LIB}
            winhttp
            ws2_32
            crypt32
        )
        
        target_link_options(FastDLTool PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
        )
        
    else()
        find_path(BZ2_INCLUDE_DIR bzlib.h)
        find_library(BZ2_LIB bz2)
        
        if(BZ2_INCLUDE_DIR AND BZ2_LIB)
            target_include_directories(FastDLTool PRIVATE ${BZ2_INCLUDE_DIR})
            target_link_libraries(FastDLTool PRIVATE ${BZ2_LIB} winhttp ws2_32 crypt32)
        else()
            message(FATAL_ERROR "BZip2 not found for MSVC")
        endif()
    endif()

else()
    find_package(CURL REQUIRED)
    find_library(BZ2_LIB NAMES bz2)
    
    if(NOT BZ2_LIB)
        message(FATAL_ERROR "libbz2 not found!")
    endif()
    
    target_link_libraries(FastDLTool PRIVATE
        CURL::libcurl
        ${BZ2_LIB}
        pthread
        dl
    )
endif()

if(MSVC)
    target_compile_options(FastDLTool PRIVATE /W3)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(FastDLTool PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS FastDLTool DESTINATION bin)

message(STATUS "")
message(STATUS "────────────────────────────────────")
message(STATUS " FastDL Tool Configuration")
message(STATUS "────────────────────────────────────")
message(STATUS " OS            : ${CMAKE_SYSTEM_NAME}")
message(STATUS " Compiler      : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " Build Type    : ${CMAKE_BUILD_TYPE}")
message(STATUS " C++ Standard  : C++${CMAKE_CXX_STANDARD}")

if(WIN32 AND MINGW)
    message(STATUS " BZip2 Include : ${BZ2_INCLUDE_DIR}")
    message(STATUS " BZip2 Library : ${BZ2_LIB}")
endif()

message(STATUS "────────────────────────────────────")
message(STATUS "")