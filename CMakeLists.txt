cmake_minimum_required( VERSION 3.28 FATAL_ERROR )

project( uid11-superbuild
	VERSION 0.1.0
	DESCRIPTION "uid11 top-level build"
	LANGUAGES CXX
)

include( CTest )   #  calls enable_testing() so ctest works from the build root

add_subdirectory( cpp )
