cmake_minimum_required(VERSION 3.10)
project(rofi-bitwarden C)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(CAIRO REQUIRED cairo)

add_library(rbw SHARED src/rofi-rbw.c)

target_include_directories(rbw PRIVATE ${GLIB_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS})
target_link_libraries(rbw ${GLIB_LIBRARIES} ${CAIRO_LIBRARIES})

set_target_properties(rbw PROPERTIES PREFIX "")

install(TARGETS rbw DESTINATION /usr/lib/rofi)
