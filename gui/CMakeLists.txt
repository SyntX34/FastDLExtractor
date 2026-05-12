cmake_minimum_required(VERSION 3.16)
project(FastDLLib VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(LIB_SOURCES
    src/fastdl_capi.cpp
    src/FastDLDownloader.cpp
    src/ConfigManager.cpp
)

add_library(fastdl SHARED ${LIB_SOURCES})

set_target_properties(fastdl PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    OUTPUT_NAME "fastdl"
)

target_include_directories(fastdl PRIVATE
    src
    third_party
)

if(WIN32)
    target_compile_definitions(fastdl PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

    if(MINGW)
        find_path(BZ2_INCLUDE_DIR NAMES bzlib.h
            PATHS "C:/msys64/mingw64/include" "C:/mingw64/include"
            NO_DEFAULT_PATH)
        find_library(BZ2_LIB NAMES libbz2.a bz2
            PATHS "C:/msys64/mingw64/lib" "C:/mingw64/lib"
            NO_DEFAULT_PATH)

        if(NOT BZ2_INCLUDE_DIR OR NOT BZ2_LIB)
            message(FATAL_ERROR "Install bzip2: pacman -S mingw-w64-x86_64-bzip2")
        endif()

        target_include_directories(fastdl PRIVATE ${BZ2_INCLUDE_DIR})
        target_link_libraries(fastdl PRIVATE ${BZ2_LIB} winhttp ws2_32 crypt32)

    else() # MSVC / vcpkg
        find_path(BZ2_INCLUDE_DIR bzlib.h)
        find_library(BZ2_LIB bz2)
        if(NOT BZ2_INCLUDE_DIR OR NOT BZ2_LIB)
            message(FATAL_ERROR "BZip2 not found for MSVC")
        endif()
        target_include_directories(fastdl PRIVATE ${BZ2_INCLUDE_DIR})
        target_link_libraries(fastdl PRIVATE ${BZ2_LIB} winhttp ws2_32 crypt32)
        target_compile_options(fastdl PRIVATE /utf-8)
    endif()

else() # Linux / macOS
    find_package(CURL REQUIRED)
    find_library(BZ2_LIB NAMES bz2)
    if(NOT BZ2_LIB)
        message(FATAL_ERROR "libbz2 not found! (apt: libbz2-dev)")
    endif()

    target_link_libraries(fastdl PRIVATE
        CURL::libcurl
        ${BZ2_LIB}
        pthread
        dl
    )
endif()

if(MSVC)
    target_compile_options(fastdl PRIVATE /W3)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fastdl PRIVATE -Wall -Wextra)
endif()

install(TARGETS fastdl DESTINATION lib)
install(FILES src/fastdl_capi.h DESTINATION include)

message(STATUS "")
message(STATUS "FastDL Shared Library")
message(STATUS "  OS       : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "")