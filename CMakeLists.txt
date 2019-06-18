cmake_minimum_required(VERSION 3.12)
project(task)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ECS_BUILD_EXAMPLES "Build documentation examples" OFF)
option(ECS_BUILD_TESTS "Build documentation tests" OFF)


include(FetchContent)

FetchContent_Declare(
	Task
	GIT_REPOSITORY "https://github.com/jedesjardins/task"
)

FetchContent_GetProperties(Task)
if(NOT Task_POPULATED)
	FetchContent_Populate(Task)
	message(STATUS "Task source dir: ${task_SOURCE_DIR}")
	message(STATUS "Task binary dir: ${task_BINARY_DIR}")
	message(STATUS "Task include dir: ${task_SOURCE_DIR}/include")
	add_subdirectory(${task_SOURCE_DIR} ${task_BINARY_DIR})
endif()

add_library(ECS INTERFACE)
target_include_directories(ECS
	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

target_link_libraries(ECS INTERFACE
	JED::task
)

add_library(JED::ecs ALIAS ECS)

if(ECS_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

if(ECS_BUILD_TESTS)
	add_subdirectory(tests)
endif()