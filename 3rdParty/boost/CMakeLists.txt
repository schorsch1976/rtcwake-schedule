#######################################################################
# Boost
#######################################################################
set(BOOST_VERSION "1.74.0")

set(CFLAGS
		BOOST_BIND_GLOBAL_PLACEHOLDERS
		BOOST_ALL_DYN_LINK=1
		BOOST_ALL_NO_LIB=1
)

find_package(Boost ${BOOST_VERSION}
	COMPONENTS
		date_time
		system
REQUIRED)

set_target_properties(	Boost::boost
						Boost::system
		PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${CFLAGS}"
)

set_target_properties(	Boost::boost
						Boost::date_time
						Boost::system
		PROPERTIES IMPORTED_GLOBAL TRUE
)


if (BUILD_TESTING)
	find_package(Boost ${BOOST_VERSION}
		COMPONENTS
			unit_test_framework
	REQUIRED)
			
	set_target_properties(	Boost::unit_test_framework
			PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${CFLAGS}"
	)
	
	set_target_properties(	Boost::unit_test_framework
			PROPERTIES IMPORTED_GLOBAL TRUE
	)
		
endif()

