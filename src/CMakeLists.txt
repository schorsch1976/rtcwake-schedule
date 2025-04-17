################################################################################
# common libs
################################################################################
set(LIBS
	Boost::date_time
	Boost::system
)

################################################################################
# The main sources
################################################################################
set(SRC_SCHEDULE
		main.cpp
		rtcwake-schedule.h
)

################################################################################
# version info
# a repackaged version could set the version string differently like a fixed one
################################################################################
execute_process(
	COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DGIT_VERSION="${GIT_VERSION}")

################################################################################
# The main executable
################################################################################
add_executable(rtcwake-schedule	${SRC_SCHEDULE} )

target_link_libraries(rtcwake-schedule PRIVATE ${LIBS})

install (TARGETS rtcwake-schedule DESTINATION bin)

################################################################################
# the test
################################################################################
if (BUILD_TESTING)
	add_executable(rtcwake-schedule-test)

	target_sources(rtcwake-schedule-test
		PRIVATE
			tests.cpp
			rtcwake-schedule.h
	)

	target_link_libraries(rtcwake-schedule-test
		PRIVATE
			Boost::unit_test_framework
	)

	add_test(rtcwake-schedule-test rtcwake-schedule-test)
endif()
