# cmake file for building LCContent
#-------------------------------------------------------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.2 FATAL_ERROR)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "LCContent requires an out-of-source build.")
endif()

# project name
if(NOT LC_CONTENT_LIBRARY_NAME STREQUAL "LCPandoraContent")
    set(LC_CONTENT_LIBRARY_NAME "LCContent")
endif()
project(${LC_CONTENT_LIBRARY_NAME})

# project version
set(${PROJECT_NAME}_VERSION_MAJOR 03)
set(${PROJECT_NAME}_VERSION_MINOR 01)
set(${PROJECT_NAME}_VERSION_PATCH 08)
set(${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

#-------------------------------------------------------------------------------------------------------------------------------------------
# Dependencies
include(PandoraCMakeSettings)

# Prefer local include directory to any paths to installed header files
include_directories(include)

find_package(PandoraSDK 03.00.00 REQUIRED)
include_directories(${PandoraSDK_INCLUDE_DIRS})
link_libraries(${PandoraSDK_LIBRARIES})
add_definitions(${PandoraSDK_DEFINITIONS})

if(PANDORA_MONITORING)
    find_package(PandoraMonitoring 03.00.00 REQUIRED)
    include_directories(${PandoraMonitoring_INCLUDE_DIRS})
    link_libraries(${PandoraMonitoring_LIBRARIES})
    add_definitions(${PandoraMonitoring_DEFINITIONS})
    add_definitions("-DMONITORING")
endif()

# Set up C++ Standard
# ``-DCMAKE_CXX_STANDARD=<standard>`` when invoking CMake
set(CMAKE_CXX_STANDARD 11 CACHE STRING "")

# Prevent CMake falls back to the latest standard the compiler does support
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#-------------------------------------------------------------------------------------------------------------------------------------------
# Low level settings - compiler etc
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -pedantic -Wno-long-long -Wno-sign-compare -Wshadow -fno-strict-aliasing ${CMAKE_CXX_FLAGS}")

include(CheckCXXCompilerFlag)
unset(COMPILER_SUPPORTS_CXX_FLAGS CACHE)
CHECK_CXX_COMPILER_FLAG(${CMAKE_CXX_FLAGS} COMPILER_SUPPORTS_CXX_FLAGS)

if(NOT COMPILER_SUPPORTS_CXX_FLAGS)
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} does not support cxx flags ${CMAKE_CXX_FLAGS}")
endif()

#-------------------------------------------------------------------------------------------------------------------------------------------
# Build products

# - Collect sources - not ideal because you have to keep running CMake to pick up changes
file(GLOB_RECURSE LC_CONTENT_SRCS RELATIVE ${PROJECT_SOURCE_DIR} "src/*.cc")

# - Add library and properties
add_library(${PROJECT_NAME} SHARED ${LC_CONTENT_SRCS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})

# - Optional documents
option(LCContent_BUILD_DOCS "Build documentation for ${PROJECT_NAME}" OFF)
if(LCContent_BUILD_DOCS)
    add_subdirectory(doc)
endif()

#-------------------------------------------------------------------------------------------------------------------------------------------
# Install products

# - library
install(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT Runtime)

# - headers
install(DIRECTORY include/ DESTINATION include COMPONENT Development FILES_MATCHING PATTERN "*.h")

# - support files
PANDORA_GENERATE_PACKAGE_CONFIGURATION_FILES(${PROJECT_NAME}Config.cmake ${PROJECT_NAME}ConfigVersion.cmake ${PROJECT_NAME}LibDeps.cmake)

#-------------------------------------------------------------------------------------------------------------------------------------------
# display some variables and write them to cache
PANDORA_DISPLAY_STD_VARIABLES()
