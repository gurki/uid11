cmake_minimum_required( VERSION 3.28 FATAL_ERROR )

project( xid
	VERSION 0.1.0
	DESCRIPTION "flexible, url-safe, single-header 64bit uid in 11 characters"
	LANGUAGES CXX
)

add_library( ${PROJECT_NAME} INTERFACE )

target_compile_features( ${PROJECT_NAME} 
    INTERFACE 
        cxx_std_23
)

target_include_directories( ${PROJECT_NAME} 
	INTERFACE 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
		$<INSTALL_INTERFACE:/>
)

option( XID_BUILD_EXAMPLES "Build xid examples" ${PROJECT_IS_TOP_LEVEL} )

if ( XID_BUILD_EXAMPLES )
	add_subdirectory( examples )
endif()