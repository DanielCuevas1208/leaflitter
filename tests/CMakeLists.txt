set(UNITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/unity)

file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.c)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src} ${UNITY_DIR}/unity.c)
    target_include_directories(${test_name} PRIVATE ${UNITY_DIR})
    target_link_libraries(${test_name} PRIVATE leaflitter::leaflitter)
    if(NOT MSVC)
        target_compile_options(${test_name} PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
