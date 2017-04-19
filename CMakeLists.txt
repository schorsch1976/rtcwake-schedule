cmake_minimum_required(VERSION 3.0)
project(rtcwake-schedule)

################################################################################
# Enable Unit tests
################################################################################
OPTION(BUILD_TESTING "Build Testing" OFF)
IF (BUILD_TESTING)
	INCLUDE(CTest)
	ENABLE_TESTING()

	################################################################################
	# boost, only required for tests
	################################################################################
	SET(Boost_USE_STATIC_LIBS ON)
	SET(Boost_ADDITIONAL_VERSIONS "1.55.0")
	FIND_PACKAGE(Boost "1.55.0" COMPONENTS system unit_test_framework REQUIRED)
ENDIF (BUILD_TESTING)

################################################################################
# general options for configuration
################################################################################
SET(RC_FILE_PATH "/etc/rtcwake-schedule/schedule" CACHE FILEPATH "Path for the schedule file")
ADD_DEFINITIONS(-DRC_FILE_PATH="${RC_FILE_PATH}")

################################################################################
# CPP FLAGS
################################################################################
IF(UNIX)
	SET(CMAKE_CXX_FLAGS   "--std=c++14 -Wall -Wno-deprecated-declarations ${CMAKE_CXX_FLAGS} -Werror=switch")

	ADD_DEFINITIONS(-DUNIX)
ENDIF(UNIX)

IF(MSVC)
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
ENDIF(MSVC)

################################################################################
# Src
################################################################################
ADD_SUBDIRECTORY(src)
