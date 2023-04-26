cmake_minimum_required(VERSION 3.17)
project(nimble_ball_presentation C)

set(CMAKE_C_STANDARD 99)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(isDebug TRUE)
else()
    message("no debug")
    set(isDebug FALSE)
endif()


if (isDebug)
    message("Debug build detected")
    add_compile_definitions(CONFIGURATION_DEBUG TYRAN_CONFIGURATION_DEBUG)
    set(CONFIGURATION_DEBUG 1)
endif()


set(DEPS ../../deps/)


file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS
        "${DEPS}*/*/src/lib/*.c"
        )

message("something ${example_src}")

add_executable(nimble_ball_presentation_example
        main.c
        ${deps_src}
)

if (WIN32)
target_link_libraries(nimble_ball_presentation_example nimble_ball_presentation)
else()
target_link_libraries(nimble_ball_presentation_example PUBLIC nimble_ball_render_sdl m)
endif(WIN32)

target_compile_options(nimble_ball_presentation_example PRIVATE -Wall -Wextra -Wshadow -Wstrict-aliasing -ansi -pedantic -Wno-declaration-after-statement -Wno-extra-semi-stmt -Wno-undef -Wno-unused-variable -Wno-unused-parameter -Wno-padded -Werror=implicit-function-declaration -Werror=incompatible-pointer-types  -Werror=missing-prototypes -Werror=int-conversion -Werror=return-type -Werror=incompatible-function-pointer-types) # -Werror=incompatible-pointer-types-discards-qualifiers
if (COMPILER_CLANG)
    target_compile_options(nimble_ball_presentation_example PRIVATE -Wmost -Weverything -Werror=missing-variable-declarations)
endif()

target_include_directories(nimble_ball_presentation_example PRIVATE ${DEPS}piot/clog/src/include)
target_include_directories(nimble_ball_presentation_example PRIVATE ${DEPS}piot/tiny-libc/src/include)
target_include_directories(nimble_ball_presentation_example PRIVATE ${DEPS}piot/transmute-c/src/include)
target_include_directories(nimble_ball_presentation_example PRIVATE ${DEPS}piot/sdl-render/src/include)
target_include_directories(nimble_ball_presentation_example PRIVATE ${DEPS}piot/nimble-ball-simulation/src/include)
target_include_directories(nimble_ball_presentation_example PRIVATE ../include)