cmake_minimum_required(VERSION 3.16)

project(leaflitter
    VERSION 1.1.0
    DESCRIPTION "A lossless byte-stream compressor that uses LZ77 matching and canonical Huffman coding"
    LANGUAGES C)

include(CTest)

option(LEAFLITTER_BUILD_TESTS "Build the unit tests" ON)
option(LEAFLITTER_BUILD_TOOLS "Build the command line tools" ON)
option(LEAFLITTER_BUILD_SHARED "Build a shared library in addition to the static library" OFF)
option(LEAFLITTER_WERROR "Treat compiler warnings as errors" OFF)

set(LEAFLITTER_SOURCES
    src/lf_adler32.c
    src/lf_bitio.c
    src/lf_block.c
    src/lf_buf.c
    src/lf_common.c
    src/lf_huffman.c
    src/lf_match.c
    src/lf_stream.c)

add_library(leaflitter STATIC ${LEAFLITTER_SOURCES})
add_library(leaflitter::leaflitter ALIAS leaflitter)

target_include_directories(leaflitter
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>)

target_compile_features(leaflitter PUBLIC c_std_11)

if(LEAFLITTER_BUILD_SHARED)
    add_library(leaflitter_shared SHARED ${LEAFLITTER_SOURCES})
    target_include_directories(leaflitter_shared
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<INSTALL_INTERFACE:include>)
    target_compile_features(leaflitter_shared PUBLIC c_std_11)
endif()

if(MSVC)
    target_compile_options(leaflitter PRIVATE /W4)
    if(LEAFLITTER_WERROR)
        target_compile_options(leaflitter PRIVATE /WX)
    endif()
    if(LEAFLITTER_BUILD_SHARED)
        target_compile_options(leaflitter_shared PRIVATE /W4)
    endif()
else()
    target_compile_options(leaflitter PRIVATE -Wall -Wextra -Wpedantic)
    if(LEAFLITTER_WERROR)
        target_compile_options(leaflitter PRIVATE -Werror)
    endif()
    if(LEAFLITTER_BUILD_SHARED)
        target_compile_options(leaflitter_shared PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(LEAFLITTER_BUILD_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(LEAFLITTER_BUILD_TOOLS)
    add_subdirectory(tools)
    add_subdirectory(samples)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(TARGETS leaflitter
    EXPORT leaflitterTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(LEAFLITTER_BUILD_SHARED)
    install(TARGETS leaflitter_shared
        EXPORT leaflitterTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")

install(EXPORT leaflitterTargets
    FILE leaflitterTargets.cmake
    NAMESPACE leaflitter::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/leaflitter)

configure_package_config_file(
    cmake/leaflitterConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/leaflitterConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/leaflitter)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/leaflitterConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/leaflitterConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/leaflitterConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/leaflitter)
