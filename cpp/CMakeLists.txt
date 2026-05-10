cmake_minimum_required( VERSION 3.28 FATAL_ERROR )

project( uid11
	VERSION 0.1.0
	DESCRIPTION "flexible, url-safe, single-header 64bit uid in 11 characters"
	HOMEPAGE_URL "https://github.com/gurki/uid11"
	LANGUAGES CXX
)

include( GNUInstallDirs )
include( CMakePackageConfigHelpers )

#-------------------------------------------------------------------------------
#  interface library
#-------------------------------------------------------------------------------

add_library( ${PROJECT_NAME} INTERFACE )
add_library( uid11::uid11 ALIAS ${PROJECT_NAME} )

target_compile_features( ${PROJECT_NAME}
	INTERFACE
		cxx_std_23
)

target_include_directories( ${PROJECT_NAME}
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

#-------------------------------------------------------------------------------
#  examples & tests (only when this project is the top-level build)
#-------------------------------------------------------------------------------

option( UID11_BUILD_EXAMPLES "Build uid11 examples" ${PROJECT_IS_TOP_LEVEL} )
option( UID11_BUILD_TESTS    "Build uid11 tests"    ${PROJECT_IS_TOP_LEVEL} )
#  Install on by default: harmless for FetchContent consumers (only triggers on
#  `cmake --install`) and convenient for everyone else. Override with -DUID11_INSTALL=OFF.
option( UID11_INSTALL        "Generate install targets for uid11" ON )

if ( UID11_BUILD_EXAMPLES )
	add_subdirectory( examples )
endif()

if ( UID11_BUILD_TESTS )
	enable_testing()
	add_subdirectory( tests )
endif()

#-------------------------------------------------------------------------------
#  install / export
#-------------------------------------------------------------------------------

if ( UID11_INSTALL )

	set( UID11_CMAKE_CONFIG_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME} )

	install( TARGETS ${PROJECT_NAME}
		EXPORT ${PROJECT_NAME}Targets
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	)

	install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILES_MATCHING PATTERN "*.h"
	)

	install( EXPORT ${PROJECT_NAME}Targets
		FILE ${PROJECT_NAME}Targets.cmake
		NAMESPACE uid11::
		DESTINATION ${UID11_CMAKE_CONFIG_DIR}
	)

	configure_package_config_file(
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
		INSTALL_DESTINATION ${UID11_CMAKE_CONFIG_DIR}
	)

	write_basic_package_version_file(
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion
		ARCH_INDEPENDENT
	)

	install( FILES
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
		DESTINATION ${UID11_CMAKE_CONFIG_DIR}
	)

endif()
