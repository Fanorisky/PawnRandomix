cmake_minimum_required(VERSION 3.19)

project(PawnRandomix
    VERSION 1.3.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================
# Build options
# =========================
option(BUILD_SAMP_PLUGIN "Build for SA-MP" OFF)

# =========================
# SA-MP build
# =========================
if(BUILD_SAMP_PLUGIN)
    message(STATUS "Building PawnRandomix for SA-MP")

    set(SAMPSDK_DIR deps/samp-plugin-sdk)

    add_library(${PROJECT_NAME} SHARED
        ${SAMPSDK_DIR}/amxplugin.cpp
        ${SAMPSDK_DIR}/amxplugin2.cpp
        ${SAMPSDK_DIR}/amx/getch.c
        main_samp.cpp
        randomix.cpp
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${SAMPSDK_DIR}
        ${SAMPSDK_DIR}/amx
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        RANDOMIX_VERSION="${PROJECT_VERSION}"
        HAVE_STDINT_H=1
        PAWN_CELL_SIZE=32
        BUILD_SAMP_PLUGIN=1
    )

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

# =========================
# open.mp build
# =========================
else()
    message(STATUS "Building PawnRandomix for open.mp")

    add_subdirectory(deps/omp-sdk)

    add_library(${PROJECT_NAME} SHARED
        main.cpp
        randomix.cpp
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
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

    # Pastikan nama target ini sesuai dengan yang ada di omp-sdk
    target_link_libraries(${PROJECT_NAME} PRIVATE OMP-SDK)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Randomix"
    )
endif()
