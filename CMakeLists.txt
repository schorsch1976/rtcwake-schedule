cmake_minimum_required(VERSION 3.0)
project(rtcwake-schedule)

SET(BOOST_REQUIRED_VERSION "1.62.0")

################################################################################
# Git is required for build (Version info)
################################################################################
FIND_PACKAGE(Git REQUIRED)
IF (GIT_FOUND)
	MESSAGE("Git: ${GIT_EXECUTABLE}")
ELSE (GIT_FOUND)
	MESSAGE(FATAL_ERROR "Git not found")
ENDIF (GIT_FOUND)

################################################################################
# Enable Unit tests
################################################################################
OPTION(BUILD_TESTING "Build Testing" OFF)
IF (BUILD_TESTING)
	INCLUDE(CTest)
	ENABLE_TESTING()

	FIND_PACKAGE(Boost ${BOOST_REQUIRED_VERSION} COMPONENTS
		system
		unit_test_framework
			REQUIRED)

ENDIF (BUILD_TESTING)

################################################################################
# boost
################################################################################
SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_ADDITIONAL_VERSIONS ${BOOST_REQUIRED_VERSION})
FIND_PACKAGE(Boost ${BOOST_REQUIRED_VERSION} COMPONENTS
	date_time system
		REQUIRED)
#ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK=1)

# no auto linking
ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

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
ADD_SUBDIRECTORY(man)
ADD_SUBDIRECTORY(src)
