cmake_minimum_required(VERSION 3.16)
project(kolemak-ime VERSION 1.0.0 LANGUAGES C)

# 버전 문자열 (프리릴리즈 접미사 지원: 5.0.0-alpha0 등)
if(DEFINED VERSION_OVERRIDE)
    set(KOLEMAK_VER "${VERSION_OVERRIDE}")
else()
    set(KOLEMAK_VER "${PROJECT_VERSION}")
endif()
configure_file(src/version.h.in "${CMAKE_BINARY_DIR}/version.h" @ONLY)

set(CMAKE_C_STANDARD 11)

# Source files
set(SOURCES
    src/globals.c
    src/dll_main.c
    src/text_service.c
    src/key_handler.c
    src/edit_session.c
    src/hangul.c
    src/keymap.c
    src/settings.c
    src/langbar.c
    src/tooltip.c
    src/tray.c
)

# Build as a shared library (DLL)
add_library(kolemak SHARED ${SOURCES})

# Module definition file for DLL exports
target_sources(kolemak PRIVATE src/kolemak.def)

# Resource file (icon)
target_sources(kolemak PRIVATE src/kolemak.rc)

# Include directories
target_include_directories(kolemak PRIVATE src "${CMAKE_BINARY_DIR}")

# Link libraries
target_link_libraries(kolemak PRIVATE
    ole32
    oleaut32
    uuid
    advapi32
    user32
    shell32
    gdi32
)

# MSVC-specific settings
if(MSVC)
    target_compile_definitions(kolemak PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
    )
    # Disable certain warnings for COM C code
    target_compile_options(kolemak PRIVATE /W4)
endif()

# MinGW settings
if(MINGW)
    target_compile_definitions(kolemak PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        COBJMACROS
        INITGUID
    )
    target_compile_options(kolemak PRIVATE -Wall -Wextra)
endif()

# Output name: kolemak.dll
set_target_properties(kolemak PROPERTIES
    OUTPUT_NAME "kolemak"
    PREFIX ""
)

# Install target
install(TARGETS kolemak RUNTIME DESTINATION bin)
