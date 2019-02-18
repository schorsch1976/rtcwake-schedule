################################################################################
# common libs
################################################################################
SET(LIBS
	Boost::date_time
	Boost::system
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
# a repackaged version could set the version string differently like a fixed one
################################################################################
EXECUTE_PROCESS(
	COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

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

	SET(TEST_LIBS ${LIBS}
		Boost::unit_test_framework
	)

	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	LINK_DIRECTORIES(${Boost_LIBRARY_DIR})

	ADD_EXECUTABLE(rtcwake-schedule-test	${SRC_TEST} )
	TARGET_LINK_LIBRARIES(rtcwake-schedule-test ${TEST_LIBS})

	ADD_TEST(rtcwake-schedule-test rtcwake-schedule-test)
ENDIF (BUILD_TESTING)
