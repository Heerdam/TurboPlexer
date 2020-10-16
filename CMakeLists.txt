cmake_minimum_required(VERSION 3.11)

project(TurboPlexer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb" )
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG")
endif()

include(FetchContent)

SET(HEADER
	src/public/tp_turboplexer.hpp
	src/public/tp_defines.hpp
)

add_library(turboplexer STATIC
	"${HEADER}"
	src/private/tp_tuboplexer.cpp
)

target_include_directories(turboplexer PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/public)

# ------------------ GLM ------------------
find_package (glm 0.9.9.9 QUIET)
if(glm_FOUND)
	message(STATUS "GLM found!")
else()
	message(STATUS "GLM not found! Fetching...")
	FetchContent_Declare(
			glm
			GIT_REPOSITORY https://github.com/g-truc/glm
	)
	FetchContent_GetProperties(glm)
	if(NOT glm_POPULATED)
		FetchContent_Populate(glm)
		add_subdirectory("${glm_SOURCE_DIR}" "${glm_BINARY_DIR}")
	endif()
endif ()

# ------------------ GLFW ------------------
find_package (glfw3 3.3.2 QUIET)
if(glfw3_FOUND)
	message(STATUS "GLFW found!")
else()
	message(STATUS "GLFW not found! Fetching...")
	FetchContent_Declare(
			glfw
			GIT_REPOSITORY https://github.com/glfw/glfw
	)

	FetchContent_GetProperties(glfw)
	if(NOT glfw_POPULATED)
	    FetchContent_Populate(glfw)

	    set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs" FORCE)
	    set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs" FORCE)
	    set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation" FORCE)
	    set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target" FORCE)

	    add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
	endif()
endif()

target_link_libraries(turboplexer
	glfw
	glm
)
