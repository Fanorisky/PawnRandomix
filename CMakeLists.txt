cmake_minimum_required(VERSION 3.19)

project(PawnRandomix
    VERSION 2.0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force dynamic runtime for all targets on Windows
if(MSVC)
    # Set before any targets are created
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "" FORCE)
    
    # Also set flags directly
    foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                     CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                     CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                     CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MT")
            string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
        endif()
        if(${flag_var} MATCHES "/MTd")
            string(REGEX REPLACE "/MTd" "/MDd" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

# Build options
option(BUILD_SAMP_PLUGIN "Build for SA-MP" OFF)

# SA-MP build
if(BUILD_SAMP_PLUGIN)
    message(STATUS "Building PawnRandomix for SA-MP")

    set(SAMPSDK_DIR deps/samp-plugin-sdk)

    # getch.c is POSIX-only (has #ifndef WIN32), exclude on Windows
    if(WIN32)
        set(SAMPSDK_SOURCES
            ${SAMPSDK_DIR}/amxplugin.cpp
            ${SAMPSDK_DIR}/amxplugin2.cpp
        )
    else()
        set(SAMPSDK_SOURCES
            ${SAMPSDK_DIR}/amxplugin.cpp
            ${SAMPSDK_DIR}/amxplugin2.cpp
            ${SAMPSDK_DIR}/amx/getch.c
        )
    endif()

    add_library(${PROJECT_NAME} SHARED
        ${SAMPSDK_SOURCES}
        src/main_samp.cpp
        src/randomix.cpp
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SAMPSDK_DIR}
        ${SAMPSDK_DIR}/amx
        src
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        RANDOMIX_VERSION="${PROJECT_VERSION}"
        HAVE_STDINT_H=1
        PAWN_CELL_SIZE=32
        BUILD_SAMP_PLUGIN=1
    )

    if(MSVC)
        # Force /MD for this target
        target_compile_options(${PROJECT_NAME} PRIVATE "/MD")
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
        
        # Hide symbols to reduce size
        target_compile_options(${PROJECT_NAME} PRIVATE "/Gy")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS "/OPT:REF /OPT:ICF"
        )
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -m32)
        target_link_options(${PROJECT_NAME} PRIVATE -m32)
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Randomix"
    )

    if(WIN32)
        set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dll")

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin.def")
            target_sources(${PROJECT_NAME} PRIVATE plugin.def)
        endif()
    endif()

# open.mp build
else()
    message(STATUS "Building PawnRandomix for open.mp")

    add_subdirectory(deps/omp-sdk)

    add_library(${PROJECT_NAME} SHARED
        src/main.cpp
        src/randomix.cpp
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        src
        deps
        deps/pawn/source
        deps/pawn-natives
    )

    if(UNIX AND NOT APPLE)
        target_include_directories(${PROJECT_NAME} PRIVATE
            deps/pawn/source/linux
        )
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        RANDOMIX_VERSION="${PROJECT_VERSION}"
        HAVE_STDINT_H=1
        PAWN_CELL_SIZE=32
    )

    if(MSVC)
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -m32)
        target_link_options(${PROJECT_NAME} PRIVATE -m32)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE OMP-SDK)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Randomix"
    )
endif()
