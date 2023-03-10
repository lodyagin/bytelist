CMAKE_MINIMUM_REQUIRED(VERSION 3.13.4)

cmake_policy(SET CMP0014 NEW)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0079 NEW)

project(bytelist C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_DEFAULT 20)
set(CMAKE_CXX_EXTENSIONS OFF)

# set(LIBCXX_GENERATED_INCLUDE_TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-multichar")

if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
	message(WARNING "\
Setting CMAKE_BUILD_TYPE to 'Debug': \
If you want to build in Release mode, you'll have to RERUN cmake with -DCMAKE_BUILD_TYPE=Release.")
	set(CMAKE_BUILD_TYPE "Debug")
endif()

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Building in debug mode.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG=1")
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
  message("Building in release mode.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRELEASE=1")
  add_flag(-s) # strip symbol table
  add_flag(-fvisibility=hidden) # hide functions' names, etc.
ENDIF()

# some compile-time flags
#include(cmake/flags.cmake)

#include(GNUInstallDirs)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


include_directories(.)

if (BUILD_TESTS)
	set(LIBCXX_INCLUDE_TESTS 1)
	add_subdirectory(tests.unit)
	add_subdirectory(tests)
endif()
