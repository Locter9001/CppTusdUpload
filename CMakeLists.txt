cmake_minimum_required(VERSION 3.26)
project(TusTestUpload C)

set(CMAKE_C_STANDARD 11)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "F:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(TusTestUpload main.c)

target_link_libraries(TusTestUpload PRIVATE CURL::libcurl OpenSSL::SSL OpenSSL::Crypto)