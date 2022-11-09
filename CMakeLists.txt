cmake_minimum_required( VERSION 3.11 )

project( fjord LANGUAGES C CXX )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_CURRENT_LIST_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/bin)

option(RTL_ENABLE_ASSERT "Turn on assertions cheks with RTL_ASSERT macro." OFF)
option(RTL_ENABLE_LOG "Enable logging to debug output with RTL_LOG macro." OFF)
option(RTL_ENABLE_RUNTIME_TESTS "Enable runtime tests execution at program startup." OFF)
option(RTL_ENABLE_WINAPI_DIAGNOSTICS "Enable checking the result of internal WinAPI calls." OFF)

aux_source_directory(src/ SOURCES)
aux_source_directory(src/fjord SOURCES)

add_executable( ${PROJECT_NAME} WIN32 ${SOURCES} )

find_package(rtl REQUIRED)

target_compile_definitions(${PROJECT_NAME} 
    PRIVATE      
        RTL_ENABLE_ASSERT=$<BOOL:${RTL_ENABLE_ASSERT}>
        RTL_ENABLE_LOG=$<BOOL:${RTL_ENABLE_LOG}>
        RTL_ENABLE_RUNTIME_TESTS=$<BOOL:${RTL_ENABLE_RUNTIME_TESTS}>
        RTL_ENABLE_WINAPI_DIAGNOSTICS=$<BOOL:${RTL_ENABLE_WINAPI_DIAGNOSTICS}>

        RTL_ENABLE_APPLICATION=1
        RTL_ENABLE_APPLICATION_CLOCK=1
        RTL_ENABLE_APPLICATION_CURSOR=0
        RTL_ENABLE_APPLICATION_KEYS=1
        RTL_ENABLE_APPLICATION_OSD=1
        RTL_ENABLE_HEAP=1
        RTL_ENABLE_MEMSET=0

        FJORD_ENABLE_FROM_FILES=1
        FJORD_ENABLE_BLOCKS_DUMP=0
        FJORD_ENABLE_STOP_AFTER_DECODING=0

        UNICODE
)

target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        src
)

if(MSVC)
    # TODO: Import some flags from RTL library
    target_compile_options(${PROJECT_NAME} 
        PRIVATE
            /EHa-       # Disable standard C++ stack unwinding. 
            /EHs-       # Disable C++ exception handling (no SEH exceptions).
            /FA         # Creates a listing file containing assembler code.
            /GF         # Enables string pooling
            /GL         # Enables whole program optimization.
            /GR-        # Disables run-time type information (RTTI).
            /Gs32768    # The number of bytes that local variables can occupy before a stack probe is initiated
            /GS-        # Disables checks buffer security.
        #   /Gy-        # Disables function-level linking
            /Gw         # Enables whole-program global data optimization.
            /O1         # Creates small code
        #   /Ob2        # Controls inline expansion
            /Oi-        # Generates intrinsic functions
            /Os         # Favors small code.
            /Oy         # Omits frame pointer. (x86 only)
            /Wall       # Enable all warnings, including warnings that are disabled by default.
            /wd4505     # 'function' : unreferenced local function has been removed
            /wd4514     # 'function' : unreferenced inline function has been removed

            # only fjord
            /wd4625     # 'derived class' : copy constructor was implicitly defined as deleted because a base class copy constructor is inaccessible or deleted
            /wd4626     # 'derived class' : assignment operator was implicitly defined as deleted because a base class assignment operator is inaccessible or deleted
            
            /wd4577     # 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
            /wd4710     # 'function' : function not inlined
            /wd4711     # function 'function' selected for inline expansion
            /wd5039     # pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
            /wd5045     # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
            /WX         # Treat warnings as errors.
    )

    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/Od" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    
    # TODO: Add for debug config /DEBUG # Creates debugging information.
    # TODO: Move stub.bin to RTL (and import linker command from)
    # TODO: Import some flags from RTL library
    target_link_options(${PROJECT_NAME} 
        PRIVATE
            /DYNAMICBASE:NO                                 # Specifies whether to generate an executable image that's rebased at load time by using the address space layout randomization (ASLR) feature.
            /FIXED                                          # Creates a program that can be loaded only at its preferred base address.
            /INCREMENTAL:NO                                 # Controls incremental linking.
            /LTCG                                           # Specifies link-time code generation.
            /MANIFEST:NO                                    # Creates a side-by-side manifest file and optionally embeds it in the binary.
            /MAP                                            # Creates a mapfile.
          # /MERGE:.data=.text                              # Combines sections.
          # /MERGE:.rdata=.text                             # Combines sections.
          # /MERGE:.edata=.text                             # Combines sections.       
            /NOCOFFGRPINFO
            /NODEFAULTLIB                                   # Ignores all (or the specified) default libraries when external references are resolved.
            /SAFESEH:NO                                     # Specifies that the image will contain a table of safe exception handlers
          # /SECTION:.text,ER                               # Overrides the attributes of a section.
            /STUB:${CMAKE_CURRENT_LIST_DIR}/lib/stub.bin    # Attaches an MS-DOS stub program to a Win32 program.
    )
endif()

target_link_libraries(${PROJECT_NAME} 
    rtl::rtl
)

###################################################################################################
# Typical segment names
###################################################################################################
#
# .text: Code 
# .data: Initialized data
# .bss: Uninitialized data
# .rdata: Const/read-only (and initialized) data
# .edata: Export descriptors
# .idata: Import descriptors
# .reloc: Relocation table (for code instructions with absolute addressing when
#           the module could not be loaded at its preferred base address)
# .rsrc: Resources (icon, bitmap, dialog, ...)
# .tls: __declspec(thread) data (Fails with dynamically loaded DLLs -> hard to find bugs)