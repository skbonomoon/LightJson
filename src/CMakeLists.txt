cmake_minimum_required(VERSION 2.6)
project(lightjson)

set(SOURCES
	json_parser.cpp 
)

if(MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG" CACHE STRING "Linker flasg for windows" FORCE)
	message(STATUS "link flags: " ${CMAKE_EXE_LINKER_FLAGS})
	message(STATUS "compiler flags: " ${CMAKE_CXX_FLAGS})
	message(STATUS "compiler flags for debug: " ${CMAKE_CXX_FLAGS_DEBUG})
	message(STATUS "compiler flags for Relase: " ${CMAKE_CXX_FLAGS_RELEASE})
endif()

add_library(lightjson STATIC ${SOURCES})

