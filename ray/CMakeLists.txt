CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3)
project(GLSL)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

FILE(GLOB cmakes ${CMAKE_SOURCE_DIR}/cmake/*.cmake)
FOREACH(cmake ${cmakes})
	INCLUDE(${cmake})
ENDFOREACH(cmake)

MESSAGE(STATUS "stdgl: ${stdgl_libraries}")

ADD_SUBDIRECTORY(src)

IF (EXISTS ${CMAKE_SOURCE_DIR}/sln/CMakeLists.txt)
	ADD_SUBDIRECTORY(sln)
ENDIF()
