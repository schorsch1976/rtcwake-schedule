if(UNIX)
	option(BUILD_MAN_PAGE "Build man page" ON)
	if(BUILD_MAN_PAGE)
		#add_custom_target(man ALL)

		add_custom_command(
		#	TARGET man
			COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/rtcwake-schedule.txt | gzip -9 > ${CMAKE_CURRENT_BINARY_DIR}/rtcwake-schedule.1.gz
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rtcwake-schedule.1.gz
		)

		#add_custom_command(
		#	TARGET man
		#	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/rtcwake-schedule.1.gz
		#)

		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rtcwake-schedule.1.gz DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1)
	endif()
endif()
