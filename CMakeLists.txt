cmake_minimum_required(VERSION 3.15)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version" FORCE)
    find_program(BREW_EXECUTABLE brew HINTS /usr/local/bin)
    if ("${BREW_EXECUTABLE}" STREQUAL "BREW_EXECUTABLE-NOTFOUND")
        message(STATUS "macOS: Homebrew not found.")
    else ()
        message(STATUS "macOS: Homebrew found at ${BREW_EXECUTABLE}.")
        execute_process(COMMAND ${BREW_EXECUTABLE} --prefix qt OUTPUT_VARIABLE QT_PREFIX_PATH)
        string(REGEX REPLACE "\n$" "" QT_PREFIX_PATH_STRIPPED "${QT_PREFIX_PATH}")
        if ("${QT_PREFIX_PATH}" STREQUAL "")
            message(STATUS "macOS: Homebrew: Qt not installed.")
        else ()
            list(APPEND CMAKE_PREFIX_PATH "${QT_PREFIX_PATH_STRIPPED}")
            message(STATUS "macOS: Homebrew Qt at ${QT_PREFIX_PATH_STRIPPED}.")
        endif ()
    endif ()
endif ()

enable_language(CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0092 NEW)

find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

project(qt-csd LANGUAGES CXX VERSION 0.1.0)

add_executable(${PROJECT_NAME} WIN32
    "${CMAKE_SOURCE_DIR}/csd.qrc"
    "${CMAKE_SOURCE_DIR}/csdtitlebar.cpp"
    "${CMAKE_SOURCE_DIR}/csdtitlebarbutton.cpp"
    "${CMAKE_SOURCE_DIR}/main.cpp"
)

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

get_target_property(${PROJECT_NAME}_SOURCES ${PROJECT_NAME} SOURCES)

foreach (${PROJECT_NAME}_SOURCE ${${PROJECT_NAME}_SOURCES})
    set_source_files_properties(${${PROJECT_NAME}_SOURCE} PROPERTIES COMPILE_FLAGS "${COMPILER_WARNINGS_STR}")
endforeach ()

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON AUTORCC ON)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
    "${Qt5Widgets_INCLUDE_DIRS}"
    )

if (APPLE)
    find_library(LIBOBJC "objc")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${LIBOBJC}
    )

elseif (UNIX)
    target_sources(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/linuxcsd.cpp"
    )

    find_package(Qt5X11Extras REQUIRED)
    find_library(LIBXCB "xcb" REQUIRED)

    set(QTCORE_LIB "${Qt5Core_LIBRARIES}")
    set(QTGUI_LIB "${Qt5Gui_LIBRARIES}")
    set(QTWIDGETS_LIB "${Qt5Widgets_LIBRARIES}")

    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${Qt5X11Extras_INCLUDE_DIRS})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${LIBXCB}
        ${Qt5X11Extras_LIBRARIES}
    )
else ()
    target_sources(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/qregistrywatcher.cpp"
        "${CMAKE_SOURCE_DIR}/qtwinbackports.cpp"
        "${CMAKE_SOURCE_DIR}/win32csd.cpp"
    )

    find_package(Qt5WinExtras REQUIRED)
    find_library(DWMAPI "dwmapi" REQUIRED)

    set(QTCORE_LIB "${Qt5Core_LIBRARIES}")
    set(QTGUI_LIB "${Qt5Gui_LIBRARIES}")
    set(QTWIDGETS_LIB "${Qt5Widgets_LIBRARIES}")

    target_compile_definitions(${PROJECT_NAME} PUBLIC WIN32_LEAN_AND_MEAN)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC UNICODE)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${DWMAPI}
    )
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE
    "${QTCORE_LIB}"
    "${QTGUI_LIB}"
    "${QTWIDGETS_LIB}"
)
