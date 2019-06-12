cmake_minimum_required(VERSION 3.14)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "OS X deployment target" FORCE)

    option(WITH_BREW_QT "Build with Homebrew Qt" ON)
    if (WITH_BREW_QT)
        find_program(BREW_EXECUTABLE brew HINTS /usr/local/bin)
        if (${BREW_EXECUTABLE} STREQUAL "BREW_EXECUTABLE-NOTFOUND")
            message(STATUS "macOS: No Homebrew found.")
        else ()
            message(STATUS "macOS: Found Homebrew at ${BREW_EXECUTABLE}.")
            execute_process(COMMAND ${BREW_EXECUTABLE} --prefix qt OUTPUT_VARIABLE QT_PREFIX_PATH)
            string(REGEX REPLACE "\n$" "" QT_PREFIX_PATH_STRIPPED "${QT_PREFIX_PATH}")
            set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${QT_PREFIX_PATH_STRIPPED}")
            message(STATUS "macOS: Using Qt at ${QT_PREFIX_PATH_STRIPPED}.")
        endif ()
    endif ()
endif ()

enable_language(CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0071 NEW)

find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

project(qt-csd LANGUAGES CXX VERSION 0.1.0)

add_executable(qt-csd WIN32 csdmainwindow.cpp csdtitlebarbutton.cpp csdtitlebar.cpp main.cpp)
if (WIN32)
    target_sources(qt-csd PRIVATE qregistrywatcher.cpp)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" AND NOT MSVC)
    list(APPEND COMPILER_WARNINGS
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-compat-pedantic"
        "-Wno-padded"
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list(APPEND COMPILER_WARNINGS
        "-Wall"
        "-Wcast-align"
        "-Wcast-qual"
        "-Wconversion"
        "-Wctor-dtor-privacy"
        "-Wdisabled-optimization"
        "-Wdouble-promotion"
        "-Wduplicated-branches"
        "-Wduplicated-cond"
        "-Wextra"
        "-Wformat=2"
        "-Winvalid-pch"
        "-Wlogical-op"
        "-Wmisleading-indentation"
        "-Wmissing-include-dirs"
        "-Wnoexcept"
        "-Wnon-virtual-dtor"
        "-Wnull-dereference"
        "-Wold-style-cast"
        "-Woverloaded-virtual"
        "-Woverloaded-virtual"
        "-Wpedantic"
        "-Wpointer-arith"
        "-Wredundant-decls"
        "-Wshadow"
        "-Wshift-overflow=2"
        "-Wsign-conversion"
        "-Wsized-deallocation"
        "-Wtrampolines"
        "-Wundef"
        "-Wunused"
        "-Wuseless-cast"
        "-Wvector-operation-performance"
        "-Wwrite-strings"
        "-Wzero-as-null-pointer-constant"
        "-pedantic-errors"
    )
else ()
    foreach (flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
    endforeach()
    list(APPEND COMPILER_WARNINGS
        "/W4"
    )
endif ()
string(REPLACE ";" " " COMPILER_WARNINGS_STR "${COMPILER_WARNINGS}")

get_target_property(qt-csd_SOURCES qt-csd SOURCES)

foreach (qt-csd_SOURCE ${qt-csd_SOURCES})
    set_source_files_properties(${qt-csd_SOURCE} PROPERTIES COMPILE_FLAGS "${COMPILER_WARNINGS_STR}")
endforeach ()

set_target_properties(qt-csd PROPERTIES AUTOMOC ON)

target_include_directories(qt-csd SYSTEM PRIVATE ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

target_link_libraries(qt-csd PRIVATE
    Qt5::Widgets
)

if (APPLE)
    find_library(LIBOBJC "objc")
    target_link_libraries(qt-csd PRIVATE
        ${LIBOBJC}
    )
endif ()

if (WIN32)
    find_package(Qt5WinExtras REQUIRED)

    target_compile_definitions(qt-csd PUBLIC WIN32_LEAN_AND_MEAN)
    target_compile_definitions(qt-csd PUBLIC NOMINMAX)
    target_compile_definitions(qt-csd PUBLIC UNICODE)

    find_library(DWMAPI "dwmapi")
    target_link_libraries(qt-csd PRIVATE
        ${DWMAPI}
        Qt5::WinExtras
    )
endif ()
