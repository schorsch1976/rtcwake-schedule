cmake_minimum_required(VERSION 3.20)
project(rtcwake-schedule)

cmake_policy(SET CMP0167 OLD)

if (CMAKE_BUILD_TYPE STREQUAL "")
	set (CMAKE_BUILD_TYPE "Release")
	message(STATUS "Set Buildtype to Release")
endif()

################################################################################
# Git is required for build (Version info)
################################################################################
find_package(Git REQUIRED)
if (GIT_FOUND)
	message(STATUS "Git: ${GIT_EXECUTABLE}")
else ()
	message(FATAL_ERROR "Git not found")
endif()

################################################################################
# Enable Unit tests
################################################################################
option(BUILD_TESTING "Build Testing" ON)
if (BUILD_TESTING)
	include(CTest)
	enable_testing()
endif()

################################################################################
# general options for configuration
################################################################################
set(RC_FILE_PATH "/etc/rtcwake-schedule/schedule" CACHE FILEPATH "Path for the schedule file")
add_definitions(-DRC_FILE_PATH="${RC_FILE_PATH}")

################################################################################
# CPP FLAGS
################################################################################
if(UNIX)
	set(CMAKE_CXX_FLAGS   "--std=c++14 -Wall -Wno-deprecated-declarations ${CMAKE_CXX_FLAGS} -Werror=switch")

	add_definitions(-DUNIX)
endif()

if(MSVC)
	add_definitions(-D_WIN32_WINNT=0x0501)
endif()

################################################################################
# Src
################################################################################
add_subdirectory(3rdParty)
add_subdirectory(man)
add_subdirectory(src)
