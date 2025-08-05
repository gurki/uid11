cmake_minimum_required( VERSION 3.28 FATAL_ERROR )

project( uuid11-examples
	VERSION 0.1.0
	DESCRIPTION "small showcase demos"
	LANGUAGES CXX
)

add_executable( ${PROJECT_NAME}
    main.cpp
)

target_compile_features( ${PROJECT_NAME} 
    PUBLIC 
        cxx_std_23
)

target_include_directories( ${PROJECT_NAME} 
	PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries( ${PROJECT_NAME} 
    PRIVATE
        uuid11
)