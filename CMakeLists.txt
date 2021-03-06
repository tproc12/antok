
cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)

set(CMAKE_VERBOSE_MAKEFILE 0)

project(ANTOK)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmakeModules")
message(STATUS "Using cmake module path '${CMAKE_MODULE_PATH}'")
include(CommonMacros)

enforce_out_of_source_build()

#C++ Flags
set(CMAKE_CXX_FLAGS "-g -Wall -Werror")

# redirect output files
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
message(STATUS "Using library output path '${LIBRARY_OUTPUT_PATH}'")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
message(STATUS "Using executable output path '${EXECUTABLE_OUTPUT_PATH}'")

# setup Boost
# environment variable $BOOST_ROOT is expected to point to non-standard locations
message(STATUS "")
message(STATUS ">>> Setting up Boost library.")
set(_BOOST_COMPONENTS "python")
set(Boost_USE_STATIC_LIBS    OFF)
set(Boost_USE_MULTITHREADED  OFF)
set(Boost_USE_STATIC_RUNTIME OFF)
if(     ("$ENV{BOOST_ROOT}"       STREQUAL "")
		AND ("$ENV{BOOSTROOT}"        STREQUAL "")
		AND ("$ENV{Boost_DIR}"        STREQUAL "")
		AND ("$ENV{BOOST_INCLUDEDIR}" STREQUAL "")
		AND ("$ENV{BOOST_LIBRARYDIR}" STREQUAL ""))
	set(Boost_NO_SYSTEM_PATHS OFF)
else()
	set(Boost_NO_SYSTEM_PATHS ON)
endif()
# this is a somewhat ugly hack
# the problem is that components cannot be defined as optional while
# at the same time the library is required. the third find_package
# line is needed in case the components are not found, because
# Boost_FOUND is set to FALSE.
find_package(Boost 1.50.0 REQUIRED)
find_package(Boost 1.50.0 QUIET COMPONENTS ${_BOOST_COMPONENTS})
foreach(_BOOST_COMPONENT ${_BOOST_COMPONENTS})
	string(TOUPPER ${_BOOST_COMPONENT} _BOOST_COMPONENT)
	if(Boost_${_BOOST_COMPONENT}_FOUND)
		message(STATUS "    Found Boost component ${_BOOST_COMPONENT} at "
			"'${Boost_${_BOOST_COMPONENT}_LIBRARY}'.")
	endif()
endforeach()
unset(_BOOST_COMPONENT)
unset(_BOOST_COMPONENTS)
find_package(Boost 1.50.0 REQUIRED)
if(Boost_FOUND)
	set(Boost_LIBRARY_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
	message(STATUS "Using Boost ${Boost_LIBRARY_VERSION} include directories '${Boost_INCLUDE_DIRS}'.")
	message(STATUS "Using Boost ${Boost_LIBRARY_VERSION} library directories '${Boost_LIBRARY_DIRS}'.")
else()
	message(FATAL_ERROR "Could not find Boost installation. "
		"Is environment variable BOOST_ROOT=${BOOST_ROOT} set correctly? Please read INSTALL.")
endif()

# setup ROOT includes and libraries
message(STATUS "")
message(STATUS ">>> Setting up ROOT.")
find_package(ROOT 5.34.04 REQUIRED)

# setup Python
message(STATUS "")
message(STATUS ">>> Setting up Python.")
option(USE_PYTHON "En/Disables Python bindings" OFF)
find_package(Python 2.6)
if(NOT PYTHONLIBS_FOUND)
	set(USE_PYTHON OFF)
	message(STATUS "Cannot find Python libraries. No Python bindings will be generated.")
else()
	set(USE_PYTHON ON)
	# check that Boost.Python library was found
	set(_BOOST_PYTHON_COMPONENTS "python")
	foreach(_BOOST_PYTHON_COMPONENT ${_BOOST_PYTHON_COMPONENTS})
		string(TOUPPER ${_BOOST_PYTHON_COMPONENT} _BOOST_PYTHON_COMPONENT)
		if(NOT Boost_${_BOOST_PYTHON_COMPONENT}_FOUND)
			set(USE_PYTHON OFF)
			message(STATUS "Cannot find Boost.Python library in "
				"'${Boost_LIBRARY_DIRS}'. Please read INSTALL on how to build Boost.Python.")
		else()
			list(APPEND Boost_PYTHON_LIBS ${Boost_${_BOOST_PYTHON_COMPONENT}_LIBRARY})
		endif()
	endforeach()
	unset(_BOOST_PYTHON_COMPONENTS)
	if(USE_PYTHON)
		message(STATUS "Using Boost.Python libraries '${Boost_PYTHON_LIBS}'.")
		message(STATUS "Enabling compilation of Python components.")
		add_definitions(-DUSE_PYTHON)
		set(PYTHON_LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/pyLib")
		file(MAKE_DIRECTORY ${PYTHON_LIBRARY_OUTPUT_PATH})
		file(MAKE_DIRECTORY ${PYTHON_LIBRARY_OUTPUT_PATH}/pytok)
		message(STATUS "Using python bindings output path '${PYTHON_LIBRARY_OUTPUT_PATH}'.")
	else()
		message("")
		message(WARNING "Found Python installation, but Boost.Python is not built correctly (consult INSTALL). "
			"No Python bindings will be generated.")
	endif()
endif()

# find yaml
message(STATUS "")
message(STATUS ">>> Setting up YAML.")
find_package(YamlCpp 0.5.0 REQUIRED)

set(ANTOK_LIB "antok")
if(USE_PYTHON)
	set(PYINTERFACE_LIBS
		pytok
		)
endif()
set(ANTOK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src)
set(USER_DIR ${CMAKE_SOURCE_DIR}/user)
aux_source_directory(${USER_DIR} USER_SOURCES)
add_subdirectory(src)
add_subdirectory(tools)
if(USE_PYTHON)
	add_subdirectory(pytok/bindings)
endif()

