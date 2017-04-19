################################################################################
# common libs
################################################################################
SET(LIBS
)

################################################################################
# The main sources
################################################################################
SET(SRC_SCHEDULE
		main.cpp
		rtcwake-schedule.h
)

################################################################################
# version info
################################################################################
IF(UNIX)
	EXECUTE_PROCESS(
	  COMMAND git describe --all --dirty
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE GIT_VERSION
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)
ENDIF(UNIX)

IF(MSVC)
	SET(GIT_VERSION "windows-unknown")
ENDIF(MSVC)

ADD_DEFINITIONS(-DGIT_VERSION="${GIT_VERSION}")

################################################################################
# The main executable
################################################################################
ADD_EXECUTABLE(rtcwake-schedule	${SRC_SCHEDULE} )

TARGET_LINK_LIBRARIES(rtcwake-schedule ${LIBS})
INSTALL (TARGETS rtcwake-schedule DESTINATION bin)

################################################################################
# the test
################################################################################
IF (BUILD_TESTING)
	SET(SRC_TEST
			tests.cpp
			rtcwake-schedule.h
	)

	IF(UNIX)
		
		SET(TEST_LIBS ${LIBS}
				${Boost_SYSTEM_LIBRARY}
				${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
		)
	ELSE(UNIX)
		SET(TEST_LIBS ${LIBS}
		)
	ENDIF(UNIX)

	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	LINK_DIRECTORIES(${Boost_LIBRARY_DIR})

	ADD_EXECUTABLE(rtcwake-schedule-test	${SRC_TEST} )
	TARGET_LINK_LIBRARIES(rtcwake-schedule-test ${TEST_LIBS})

	ADD_TEST(rtcwake-schedule-test rtcwake-schedule-test)
ENDIF (BUILD_TESTING)
