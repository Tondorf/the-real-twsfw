cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
        twsfw
        VERSION 0.1.0
        DESCRIPTION "Short description"
        HOMEPAGE_URL "https://example.com/"
        LANGUAGES C CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
        twsfw_twsfw
        source/game.cpp
        source/twsfwphysx_impl.c
        source/physx.cpp
)
add_library(twsfw::twsfw ALIAS twsfw_twsfw)

include(GenerateExportHeader)
generate_export_header(
        twsfw_twsfw
        BASE_NAME twsfw
        EXPORT_FILE_NAME export/twsfw/twsfw_export.hpp
        CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(twsfw_twsfw PUBLIC TWSFW_STATIC_DEFINE)
endif ()

set_target_properties(
        twsfw_twsfw PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME twsfw
        OUTPUT_NAME twsfw
)

target_include_directories(
        twsfw_twsfw ${warning_guard}
        PUBLIC
        "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
        twsfw_twsfw SYSTEM
        PUBLIC
        "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(twsfw_twsfw PUBLIC cxx_std_20)

# ---- Add dependency: wasmtime ----

set(WASMTIME_VERSION "v30.0.2")
set(WASMTIME_ARCH "x86_64-linux")
set(WASMTIME_ARTIFACT_URL "https://github.com/bytecodealliance/wasmtime/releases/download/${WASMTIME_VERSION}/wasmtime-${WASMTIME_VERSION}-${WASMTIME_ARCH}-c-api.tar.xz")
set(WASMTIME_ARTIFACT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/wasmtime-${WASMTIME_VERSION}-${WASMTIME_ARCH}-c-api.tar.xz")
set(WASMTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/wasmtime-${WASMTIME_VERSION}-${WASMTIME_ARCH}-c-api")

file(DOWNLOAD ${WASMTIME_ARTIFACT_URL} ${WASMTIME_ARTIFACT_PATH} SHOW_PROGRESS)
execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xJf ${WASMTIME_ARTIFACT_PATH}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

find_library(wasmtime NAMES wasmtime PATHS ${WASMTIME_DIR}/lib NO_DEFAULT_PATH)
if (wasmtime)
    message(STATUS "Found Wasmtime library: ${wasmtime}")
    target_include_directories(twsfw_twsfw SYSTEM PRIVATE ${WASMTIME_DIR}/include)
    target_link_libraries(twsfw_twsfw PRIVATE ${wasmtime})
else ()
    message(FATAL_ERROR "Wasmtime library not found!")
endif ()

# ---- Add dependency: twsfwphysx ----

add_subdirectory(twsfwphysx)
target_link_libraries(
        twsfw_twsfw PUBLIC
        twsfwphysx::twsfwphysx
)

# ---- Install rules ----

if (NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif ()

# ---- Examples ----

if (PROJECT_IS_TOP_LEVEL)
    option(BUILD_EXAMPLES "Build examples tree." "${twsfw_DEVELOPER_MODE}")
    if (BUILD_EXAMPLES)
        add_subdirectory(example)
    endif ()
endif ()

# ---- Developer mode ----

if (NOT twsfw_DEVELOPER_MODE)
    return()
elseif (NOT PROJECT_IS_TOP_LEVEL)
    message(
            AUTHOR_WARNING
            "Developer mode is intended for developers of twsfw"
    )
endif ()

include(cmake/dev-mode.cmake)
