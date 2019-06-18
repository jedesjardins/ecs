cmake_minimum_required(VERSION 3.12)
project(ecs_examples)

message(STATUS "Examples included")

# Modeled after Catch2 examples directory

# gather all example files
set(SOURCES_SINGLE_FILE
	# list test files here
	simple.cpp
	sparse_set.cpp
	step_system.cpp
	system_topology.cpp
	multithreaded_entity_space_usage.cpp
	system_data.cpp
)

# get base names of each file (strips off .cpp)
string(REPLACE ".cpp" "" BASENAMES_SINGLE_FILE "${SOURCES_SINGLE_FILE}")

# gather list of single file targets
set(TARGETS_SINGLE_FILE ${BASENAMES_SINGLE_FILE} )

foreach(name ${TARGETS_SINGLE_FILE})
    add_executable(${name} ${CMAKE_CURRENT_LIST_DIR}/${name}.cpp)
    target_link_libraries(${name}
    	JED::ecs JED::mul_utl
    )
endforeach()