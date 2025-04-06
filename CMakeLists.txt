cmake_minimum_required(VERSION 3.10)
project(L2JIMP2 C)

set(CMAKE_C_STANDARD 11)

set(CURL_ROOT "C:/MinGW/curl-8.12.1_4-win64-mingw")
include_directories("${CURL_ROOT}/include")
link_directories("${CURL_ROOT}/bin")

find_library(CURL_LIBRARY NAMES curl libcurl libcurl-x64 PATHS "${CURL_ROOT}/lib")
if(NOT CURL_LIBRARY)
    message(FATAL_ERROR "cURL library not found!")
endif()

# Add the executable
add_executable(L2JIMP2 main.c)

# Link cURL and required Windows libraries
target_link_libraries(L2JIMP2 ${CURL_LIBRARY} ws2_32 crypt32)
