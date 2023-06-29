cmake_minimum_required(VERSION 3.11)

project(fjord LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/bin)

option(RTL_ENABLE_ASSERT "Turn on assertions cheks with RTL_ASSERT macro." OFF)
option(RTL_ENABLE_APP_FULLSCREEN "Start app in fullscreen mode." OFF)
option(RTL_ENABLE_APP_RESIZE "Enable application window resizing." OFF)
option(RTL_ENABLE_LOG "Enable logging to debug output with RTL_LOG macro." OFF)
option(RTL_ENABLE_RUNTIME_CHECKS "Enable checking the result of system API's calls." OFF)
option(RTL_ENABLE_RUNTIME_TESTS "Enable runtime tests execution at program startup." OFF)

aux_source_directory(src/ SOURCES)
aux_source_directory(src/fjord SOURCES)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        RTL_ENABLE_APP ON
        RTL_ENABLE_APP_OSD ON
        RTL_ENABLE_APP_CURSOR ON
        RTL_ENABLE_APP_CLOCK ON
        RTL_ENABLE_APP_FULLSCREEN ${RTL_ENABLE_APP_FULLSCREEN}
        RTL_ENABLE_APP_KEYS ON
        RTL_ENABLE_APP_RESIZE ${RTL_ENABLE_APP_RESIZE}
        RTL_ENABLE_APP_SCREEN_BUFFER ON
        RTL_ENABLE_APP_SINGLETON ON
        RTL_ENABLE_ASSERT ${RTL_ENABLE_ASSERT}
        RTL_ENABLE_HEAP ON
        RTL_ENABLE_LOG ${RTL_ENABLE_LOG}
        RTL_ENABLE_RUNTIME_CHECKS ${RTL_ENABLE_RUNTIME_CHECKS}
        RTL_ENABLE_RUNTIME_TESTS ${RTL_ENABLE_RUNTIME_TESTS}
)

find_package(rtl REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE rtl::rtl)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        FJORD_ENABLE_FROM_FILES=1
        FJORD_ENABLE_BLOCKS_DUMP=0
        FJORD_ENABLE_STOP_AFTER_DECODING=0
)

if(MSVC)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()
