cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS false)


# --- Detect CMake build type, compiler and operating system ---

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("detected debug build")
  set(isDebug TRUE)
else()
  message("detected release build")
  set(isDebug FALSE)
endif()


if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(COMPILER_NAME "clang")
  set(COMPILER_CLANG 1)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(COMPILER_NAME "gcc")
  set(COMPILER_GCC 1)
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  set(COMPILER_NAME "msvc")
  set(COMPILER_MSVC 1)
endif()

message("detected compiler: '${CMAKE_C_COMPILER_ID}' (${COMPILER_NAME})")

set(useSanitizers false)

if(useSanitizers)
  message("using sanitizers")
  set(sanitizers "-fsanitize=address")
endif()


# --- Library ---


file(GLOB lib_src FOLLOW_SYMLINKS "./*.c")


add_library(nimble-ball-presentation STATIC ${lib_src})

find_package(SDL2 REQUIRED COMPONENTS SDL2)
find_package(SDL2_image REQUIRED COMPONENTS SDL2_image>=2.0.0)
find_package(SDL2_ttf REQUIRED COMPONENTS SDL2_ttf>=2.0.0)
find_package(SDL2_mixer REQUIRED COMPONENTS SDL2_mixer>=2.0.0)
target_link_libraries(nimble-ball-presentation PUBLIC SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf SDL2_mixer::SDL2_mixer)


if(APPLE)
  target_compile_definitions(nimble-ball-presentation PRIVATE TORNADO_OS_MACOS)
  set(OS_MACOS 1)
elseif(UNIX)
  target_compile_definitions(nimble-ball-presentation PRIVATE TORNADO_OS_LINUX)
  set(OS_LINUX 1)
elseif(WIN32)
  target_compile_definitions(nimble-ball-presentation PRIVATE TORNADO_OS_WINDOWS)
  set(OS_WINDOWS 1)

endif()

if(isDebug)
  message("Setting definitions based on debug")
  target_compile_definitions(nimble-ball-presentation PRIVATE CONFIGURATION_DEBUG)
endif()


function(unixlike)
endfunction()

if(OS_LINUX)
  message("Linux Detected!")
  unixlike()

elseif(OS_MACOS)
  message("MacOS detected!")
  unixlike()
endif()

target_include_directories(nimble-ball-presentation PUBLIC ../include)

if(COMPILER_CLANG)
  target_compile_options(
    nimble-ball-presentation
    PRIVATE -Weverything
            -Werror
            -Wno-padded # the order of the fields in struct can matter (ABI)
            -Wno-declaration-after-statement # must be bug in clang? shows up
                                             # even if std=c99
            -Wno-unsafe-buffer-usage # unclear why it fails on clang-16
            -Wno-unknown-warning-option # support newer clang versions, e.g. clang-16
            -Wno-disabled-macro-expansion
            ${sanitizers})
elseif(COMPILER_GCC)
target_compile_options(
    nimble-ball-presentation
    PRIVATE -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wno-padded # the order of the fields in struct can matter (ABI)
            ${sanitizers})
elseif(COMPILER_MSVC)
  target_compile_options(
    nimble-ball-presentation
    PRIVATE /F
            5000000
            /Wall
            /wd4668
            /wd4820
            /wd4100
            /wd4710
            /wd4711
            /wd5945
            /wd5105
            /wd5045
            /we4047
            /we4255
            /we4013
            /we4020
            /we4474
            /we4477
            /we4057
            /we4101
            /we4189) # /WX
else()
  target_compile_options(nimble-ball-presentation PRIVATE -Wall)
endif()


target_link_libraries(nimble-ball-presentation PUBLIC sdl-render PUBLIC nimble-ball-simulation)


if(useSanitizers)
  target_link_libraries(nimble-ball-presentation PRIVATE ${sanitizers})
endif()

if(NOT isDebug)
  message("optimize!")
  target_compile_options(nimble-ball-presentation PRIVATE -O3)
endif()

if (NOT MSVC)
target_link_libraries(nimble-ball-presentation PRIVATE m)
endif()
