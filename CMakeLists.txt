cmake_minimum_required (VERSION 3.1)
project(pfs-acpi C CXX)

option(pfs-acpi_BUILD_DEMO "Build Demo" ON)

set(SOURCES)
list(APPEND INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/3rdparty")

if (UNIX AND NOT APPLE) # AND NOT CYGWIN
    set(LINUX TRUE)
endif()

if (LINUX)
    set(LIBACPI_ROOT "${CMAKE_SOURCE_DIR}/3rdparty/libacpi-0.2")
    list(APPEND SOURCES "${LIBACPI_ROOT}/libacpi.c" "${LIBACPI_ROOT}/list.c")
endif()

list(REMOVE_DUPLICATES INCLUDE_DIRS)

message(STATUS "SOURCES: [${SOURCES}]")

add_library(pfs-acpi ${SOURCES})
target_include_directories(pfs-acpi PUBLIC ${INCLUDE_DIRS})
set_target_properties(pfs-acpi
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if (pfs-acpi_BUILD_DEMO)
    add_subdirectory(demo)
endif()
