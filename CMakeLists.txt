cmake_minimum_required(VERSION 3.19)

project(PawnRandomix
    VERSION 2.0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC specific settings
if(MSVC)
    # Use dynamic runtime (not static)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    
    # Release mode optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /DNDEBUG /MD")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Oi /DNDEBUG /MD")
    
    # Remove debug flags if present
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/ZI" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/Zi" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/ZI" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    
    # Linker optimizations
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
endif()

# Build options
option(BUILD_SAMP_PLUGIN "Build for SA-MP" OFF)

# SA-MP build
if(BUILD_SAMP_PLUGIN)
    message(STATUS "Building PawnRandomix for SA-MP")

    set(SAMPSDK_DIR deps/samp-plugin-sdk)

    add_library(${PROJECT_NAME} SHARED
        ${SAMPSDK_DIR}/amxplugin.cpp
        ${SAMPSDK_DIR}/amxplugin2.cpp
        ${SAMPSDK_DIR}/amx/getch.c
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

    if(NOT MSVC)
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

    if(NOT MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE -m32)
        target_link_options(${PROJECT_NAME} PRIVATE -m32)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE OMP-SDK)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Randomix"
    )
endif()
