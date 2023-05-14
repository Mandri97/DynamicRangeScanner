cmake_minimum_required(VERSION 3.2)

project(dynamic_range_scanner)

# Compiler flags
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-O3 -Wall")

# FFmpeg config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET 
	libavcodec 
	libavformat
)

file(GLOB sources ${CMAKE_SOURCE_DIR}/src/*.cc)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_library(DRS_LIB SHARED ${sources})
#target_link_libraries(DRS_LIB PkgConfig::LIBAV)


#target_link_libraries(MAIN_DRS_LIB INTERFACE INT_DRS_LIB PkgConfig::LIBAV)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC DRS_LIB PkgConfig::LIBAV)
