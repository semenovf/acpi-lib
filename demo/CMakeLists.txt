cmake_minimum_required (VERSION 3.1)

set(DEMOS
    acpi_demo)

foreach (demo ${DEMOS})
    file(GLOB SOURCES ${demo}/*.cpp)
    add_executable(${demo} ${SOURCES})
    target_link_libraries(${demo} pfs-acpi)

    set_target_properties(${demo}
        PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endforeach()
