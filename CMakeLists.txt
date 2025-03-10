cmake_minimum_required(VERSION 3.12)

include(ExternalProject)
find_package(Git REQUIRED)
find_package(Threads REQUIRED)
find_program(iwyu_path OPTIONAL NAMES include-what-you-use iwyu)
find_program(clang_tidy_path OPTIONAL NAMES clang-tidy)

# depot_tools
# -----------------------------------------------------------------------------

ExternalProject_Add(
  project_depot_tools

  GIT_REPOSITORY    https://chromium.googlesource.com/chromium/tools/depot_tools.git
  GIT_TAG           2fddb95698211db1373ebe2b16091a54eac51c9c

  PREFIX            ${CMAKE_BINARY_DIR}/external/depot_tools/prefix
  TMP_DIR           ${CMAKE_BINARY_DIR}/external/depot_tools/tmp
  STAMP_DIR         ${CMAKE_BINARY_DIR}/external/depot_tools/stamp
  DOWNLOAD_DIR      ${CMAKE_BINARY_DIR}/external/depot_tools/download
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/external/depot_tools/src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/external/depot_tools/build

  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
)

ExternalProject_Get_Property(project_depot_tools SOURCE_DIR)
set(depot_tools_install_dir ${SOURCE_DIR})

# libc++
# -----------------------------------------------------------------------------

set(libwebrtc_binary_dir ${CMAKE_BINARY_DIR}/external/libwebrtc/build/${CMAKE_BUILD_TYPE})
set(libwebrtc_src_dir ${CMAKE_BINARY_DIR}/external/libwebrtc/download/src)

add_library(libc++ OBJECT IMPORTED)
add_dependencies(libc++ libwebrtc)

set(libc++_objects
  algorithm.o
  any.o
  bind.o
  charconv.o
  chrono.o
  condition_variable.o
  condition_variable_destructor.o
  debug.o
  exception.o
  functional.o
  future.o
  hash.o
  ios.o
  iostream.o
  locale.o
  memory.o
  mutex.o
  mutex_destructor.o
  new.o
  optional.o
  random.o
  regex.o
  shared_mutex.o
  stdexcept.o
  string.o
  strstream.o
  system_error.o
  thread.o
  typeinfo.o
  utility.o
  valarray.o
  variant.o
  vector.o
)
list(TRANSFORM libc++_objects PREPEND ${libwebrtc_binary_dir}/obj/buildtools/third_party/libc++/libc++/)

set_property(TARGET libc++ APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libc++ PROPERTIES IMPORTED_OBJECTS_DEBUG "${libc++_objects}" IMPORTED_OBJECTS "${libc++_objects}")

# NOTE(mroberts): I would like this to be INTERFACE.
#
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
#
# target_include_directories(libc++ SYSTEM INTERFACE "${libc++_include_dir}")

# libc++abi
# -----------------------------------------------------------------------------

add_library(libc++abi OBJECT IMPORTED)
add_dependencies(libc++abi libwebrtc)

set(libc++abi_objects
  abort_message.o
  cxa_aux_runtime.o
  cxa_default_handlers.o
  cxa_demangle.o
  cxa_exception.o
  cxa_exception_storage.o
  cxa_guard.o
  cxa_handlers.o
  cxa_personality.o
  cxa_unexpected.o
  cxa_vector.o
  cxa_virtual.o
  fallback_malloc.o
  private_typeinfo.o
  stdlib_exception.o
  stdlib_stdexcept.o
  stdlib_typeinfo.o
)
list(TRANSFORM libc++abi_objects PREPEND ${libwebrtc_binary_dir}/obj/buildtools/third_party/libc++abi/libc++abi/)

set_property(TARGET libc++abi APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libc++abi PROPERTIES IMPORTED_OBJECTS_DEBUG "${libc++abi_objects}" IMPORTED_OBJECTS "${libc++abi_objects}")

# NOTE(mroberts): I would like this to be INTERFACE.
#
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
#
# target_include_directories(libc++abi SYSTEM INTERFACE "${libc++abi_include_dir}")

# libwebrtc
# -----------------------------------------------------------------------------

set(WEBRTC_REVISION branch-heads/4280)

list(APPEND GN_GEN_ARGS
  rtc_build_examples=false
  rtc_use_x11=false
  rtc_enable_protobuf=false
  rtc_use_gtk=false
  rtc_include_pulse_audio=false
  rtc_include_tests=false
)
if ("$ENV{TARGET_ARCH}" STREQUAL "arm")
  list(APPEND GN_GEN_ARGS
    target_os="linux"
    target_cpu="arm"
    rtc_build_tools=true
    treat_warnings_as_errors=false
    fatal_linker_warnings=false
  )
elseif ("$ENV{TARGET_ARCH}" STREQUAL "arm64")
  list(APPEND GN_GEN_ARGS
    target_os="linux"
    target_cpu="arm64"
    rtc_build_tools=true
    treat_warnings_as_errors=false
    fatal_linker_warnings=false
  )
else()
  list(APPEND GN_GEN_ARGS
    rtc_build_tools=false
  )
endif()

if (WIN32)
  list(APPEND GN_GEN_ARGS is_clang=false)
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND GN_GEN_ARGS is_debug=true)
else()
  list(APPEND GN_GEN_ARGS is_debug=false)
endif()
string(REPLACE ";" " " GN_GEN_ARGS "${GN_GEN_ARGS}")

if(WIN32)
  set(suffix bat)
  set(PLATFORM windows)
else()
  set(suffix sh)
  if(APPLE)
    set(PLATFORM darwin)
  else()
    set(PLATFORM linux)
  endif()
endif()

if (WIN32)
  set(byproducts
    ${libwebrtc_binary_dir}/obj/webrtc.lib
    ${libwebrtc_binary_dir}/obj/pc/peerconnection.lib
  )
else()
  set(byproducts
    ${libc++_objects}
    ${libc++abi_objects}
    ${libwebrtc_binary_dir}/obj/libwebrtc.a
    ${libwebrtc_binary_dir}/obj/pc/libpeerconnection.a
  )
endif()

ExternalProject_Add(
  project_libwebrtc

  PREFIX            ${CMAKE_BINARY_DIR}/external/libwebrtc/prefix
  TMP_DIR           ${CMAKE_BINARY_DIR}/external/libwebrtc/tmp
  STAMP_DIR         ${CMAKE_BINARY_DIR}/external/libwebrtc/stamp
  DOWNLOAD_DIR      ${CMAKE_BINARY_DIR}/external/libwebrtc/download
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/external/libwebrtc/download/src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/external/libwebrtc/build/${CMAKE_BUILD_TYPE}

  BUILD_BYPRODUCTS  ${byproducts}

  DOWNLOAD_COMMAND  ${CMAKE_COMMAND} -E env DEPOT_TOOLS=${depot_tools_install_dir} PLATFORM=${PLATFORM} WEBRTC_REVISION=${WEBRTC_REVISION} ${CMAKE_SOURCE_DIR}/scripts/download-webrtc.${suffix}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env BINARY_DIR=<BINARY_DIR> DEPOT_TOOLS=${depot_tools_install_dir} GN_GEN_ARGS=${GN_GEN_ARGS} SOURCE_DIR=<SOURCE_DIR> ${CMAKE_SOURCE_DIR}/scripts/configure-webrtc.${suffix}
  BUILD_COMMAND     ${CMAKE_COMMAND} -E env CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} DEPOT_TOOLS=${depot_tools_install_dir} ${CMAKE_SOURCE_DIR}/scripts/build-webrtc.${suffix}
  INSTALL_COMMAND   ""
)

add_dependencies(project_libwebrtc project_depot_tools)

ExternalProject_Get_Property(project_libwebrtc DOWNLOAD_DIR)
set(libwebrtc_source_dir "${DOWNLOAD_DIR}")

ExternalProject_Get_Property(project_libwebrtc BINARY_DIR)
set(libwebrtc_binary_dir "${BINARY_DIR}")

add_library(libwebrtc STATIC IMPORTED)
add_dependencies(libwebrtc project_libwebrtc)

if(WIN32)
  set_property(TARGET libwebrtc PROPERTY IMPORTED_LOCATION "${libwebrtc_binary_dir}/obj/webrtc.lib")
else()
  set_property(TARGET libwebrtc PROPERTY IMPORTED_LOCATION "${libwebrtc_binary_dir}/obj/libwebrtc.a")
endif()

add_library(libpeerconnection STATIC IMPORTED)
add_dependencies(libpeerconnection project_libwebrtc)

if(WIN32)
  set_property(TARGET libpeerconnection PROPERTY IMPORTED_LOCATION "${libwebrtc_binary_dir}/obj/pc/peerconnection.lib")
else()
  set_property(TARGET libpeerconnection PROPERTY IMPORTED_LOCATION "${libwebrtc_binary_dir}/obj/pc/libpeerconnection.a")
endif()

set(libc++_include_dir "${libwebrtc_source_dir}/src/buildtools/third_party/libc++/trunk/include")
set(libc++abi_include_dir "${libwebrtc_source_dir}/src/buildtools/third_party/libc++abi/trunk/include")

# NOTE(mroberts): I would like this to be INTERFACE.
#
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
#
# target_include_directories(libwebrtc SYSTEM INTERFACE
#   ${libwebrtc_source_dir}
#   ${libwebrtc_source_dir}/webrtc
#   ${libwebrtc_source_dir}/webrtc/third_party/abseil-cpp
#   ${libwebrtc_source_dir}/webrtc/third_party/libyuv/include
# )

# catch2
# -----------------------------------------------------------------------------

ExternalProject_Add(
  project_catch2

  GIT_REPOSITORY    https://github.com/catchorg/Catch2.git
  GIT_TAG           a1cdff4f188170c5b0aa3d5d217b60d821f237df

  PREFIX            ${CMAKE_BINARY_DIR}/external/catch2/prefix
  TMP_DIR           ${CMAKE_BINARY_DIR}/external/catch2/tmp
  STAMP_DIR         ${CMAKE_BINARY_DIR}/external/catch2/stamp
  DOWNLOAD_DIR      ${CMAKE_BINARY_DIR}/external/catch2/download
  SOURCE_DIR        ${CMAKE_BINARY_DIR}/external/catch2/src
  BINARY_DIR        ${CMAKE_BINARY_DIR}/external/catch2/build

  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
)

ExternalProject_Get_Property(project_catch2 SOURCE_DIR)
set(catch2_install_dir ${SOURCE_DIR})

# node-webrtc
# -----------------------------------------------------------------------------

set(MODULE wrtc)
include(${CMAKE_SOURCE_DIR}/NodeJS.cmake)
if(DEFINED ENV{npm_config_runtime})
  if($ENV{npm_config_runtime} STREQUAL "electron")
    if(NOT DEFINED ENV{npm_config_target})
      message(FATAL_ERROR "npm_config_target must be defined for electron build")
    endif()
    list(APPEND nodejs_init_args "VERSION" v$ENV{npm_config_target})
    if(NOT DEFINED ENV{npm_config_disturl})
      message(FATAL_ERROR "npm_config_disturl must be defined for electron build")
    endif()
    list(APPEND nodejs_init_args "URL" $ENV{npm_config_disturl})
    if($ENV{npm_config_target} STRLESS "4")
      list(APPEND nodejs_init_args "NAME" "iojs")
    endif()
  endif()
endif()
nodejs_init(${nodejs_init_args})

file(GLOB_RECURSE MODULE_SRC src/*.cc src/*.h)
add_nodejs_module(${MODULE} ${MODULE_SRC})
set_property(TARGET ${MODULE} PROPERTY CXX_STANDARD 14)

if(NOT iwyu_path STREQUAL "iwyu_path-NOTFOUND")
  set_property(TARGET ${MODULE} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path} -Xiwyu --mapping_file=${CMAKE_SOURCE_DIR}/.iwyu.imp)
endif()

if(NOT clang_tidy_path STREQUAL "clang_tidy_path-NOTFOUND")
  set_property(TARGET ${MODULE} PROPERTY CXX_CLANG_TIDY ${clang_tidy_path})
endif()

set(node_addon_api_dir ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api)

# NOTE(mroberts): Workaround for
#
#   https://gitlab.kitware.com/cmake/cmake/issues/15052
#
# is to include all the header files here.
target_include_directories(${MODULE} SYSTEM PRIVATE
  ${node_addon_api_dir}/..
  ${libwebrtc_source_dir}
  ${libwebrtc_source_dir}/webrtc
  ${libwebrtc_source_dir}/webrtc/third_party/abseil-cpp
  ${libwebrtc_source_dir}/webrtc/third_party/libyuv/include
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_dependencies(${MODULE} project_catch2)
  target_include_directories(${MODULE} SYSTEM PRIVATE
    ${catch2_install_dir}/single_include
  )
endif()

target_include_directories(${MODULE} PRIVATE
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${MODULE} PRIVATE
  ${CMAKE_THREAD_LIBS_INIT}
  libpeerconnection
  libwebrtc
)

target_compile_definitions(${MODULE} PRIVATE
  -DNAPI_VERSION=3
  -DUSE_BUILTIN_SW_CODECS
)

if(WIN32)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /GR-")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /GR- -D_HAS_ITERATOR_DEBUGGING=0")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /FORCE:UNRESOLVED")

  target_link_libraries(${MODULE} PRIVATE
    dmoguids.lib
    msdmo.lib
    secur32.lib
    winmm.lib
    wmcodecdspuuid.lib
    ws2_32.lib
  )

  target_compile_definitions(${MODULE} PRIVATE
    -D_WINSOCKAPI_
    -DINCL_EXTRA_HTON_FUNCTIONS
    -DNOGDI
    -DNOMINMAX
    -DWEBRTC_WIN
  )
else()
  # NOTE(mroberts): Workaround for
  #
  #   https://gitlab.kitware.com/cmake/cmake/issues/15052
  #
  # is to include all the header files here.
  target_include_directories(${MODULE} SYSTEM PRIVATE
    ${libc++_include_dir}
    ${libc++abi_include_dir}
  )

  # NOTE(mroberts): On Linux and macOS, we statically link the same libc++ and
  # libc++abi libraries that libwebrtc builds against.
  target_link_libraries(${MODULE} PRIVATE
    -lc
    -lm
    libc++
    libc++abi
  )

  target_compile_options(${MODULE} PRIVATE
    -fno-rtti
    -fvisibility=hidden
    -nostdinc++
    -nodefaultlibs
    -Wall
    -Wextra
    -Wno-missing-field-initializers
    -Wno-uninitialized
    -Wno-unused-local-typedefs
    -Wno-unused-variable
    -Wpedantic
  )

  target_compile_definitions(${MODULE} PRIVATE
    -D_LIBCPP_ABI_UNSTABLE
    -DWEBRTC_POSIX=1
  )

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${MODULE} PRIVATE
      -g
    )

    target_compile_definitions(${MODULE} PRIVATE
      -DDEBUG
    )
  endif()

  if(APPLE)
    target_link_libraries(${MODULE} PRIVATE
      "-framework AppKit"
      "-framework AVFoundation"
    )

    target_compile_options(${MODULE} PRIVATE
      -Weverything
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-documentation
      -Wno-exit-time-destructors
      -Wno-float-conversion
      -Wno-global-constructors
      -Wno-padded
      -Wno-shadow
      -Wno-shadow-field-in-constructor
      -Wno-shorten-64-to-32
      -Wno-sign-conversion
      -Wno-thread-safety-negative
      -Wno-unused-template
      -Wno-weak-vtables
    )

    target_compile_definitions(${MODULE} PRIVATE
      -D_LIBCPP_ABI_UNSTABLE
      -DWEBRTC_MAC
      -DWEBRTC_IOS
    )
  else()
    target_compile_definitions(${MODULE} PRIVATE
      -DWEBRTC_LINUX
    )

    target_compile_options(${MODULE} PRIVATE
      -fpermissive
    )

    if ("$ENV{TARGET_ARCH}" STREQUAL "arm" OR "$ENV{TARGET_ARCH}" STREQUAL "arm64")
      set(CMAKE_SYSTEM_NAME Linux)
      set(CMAKE_SYSTEM_PROCESSOR "$ENV{TARGET_ARCH}")
      set(tools $ENV{ARM_TOOLS_PATH})

      set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
      set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

      if ("$ENV{TARGET_ARCH}" STREQUAL "arm")
        set(CMAKE_SYSROOT ${libwebrtc_source_dir}/src/build/linux/debian_sid_arm-sysroot)
        target_compile_options(${MODULE} PRIVATE
          -march=armv7-a
          -mfloat-abi=hard
          -mtune=generic-armv7-a
          -mfpu=neon
        )
        set(CMAKE_C_COMPILER ${tools}/bin/arm-linux-gnueabihf-gcc)
        set(CMAKE_CXX_COMPILER ${tools}/bin/arm-linux-gnueabihf-g++)
      else()
        set(CMAKE_SYSROOT ${libwebrtc_source_dir}/src/build/linux/debian_sid_arm64-sysroot)
        set(CMAKE_C_COMPILER ${tools}/bin/aarch64-linux-gnu-gcc)
        set(CMAKE_CXX_COMPILER ${tools}/bin/aarch64-linux-gnu-g++)
      endif()
    else()
      target_compile_options(${MODULE} PRIVATE
        -B${libwebrtc_source_dir}/src/third_party/binutils/Linux_x64/Release/bin
        --sysroot=${libwebrtc_source_dir}/src/build/linux/debian_sid_amd64-sysroot
      )
    endif()
  endif()
endif()

# astyle
# -----------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND ASTYLE_CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}")

  ExternalProject_Add(
    astyle
    GIT_REPOSITORY      https://github.com/Bareflank/astyle.git
    GIT_TAG             v1.2
    GIT_SHALLOW         true
    CMAKE_ARGS          ${ASTYLE_CMAKE_ARGS}
    PREFIX              ${CMAKE_BINARY_DIR}/external/astyle/prefix
    TMP_DIR             ${CMAKE_BINARY_DIR}/external/astyle/tmp
    STAMP_DIR           ${CMAKE_BINARY_DIR}/external/astyle/stamp
    DOWNLOAD_DIR        ${CMAKE_BINARY_DIR}/external/astyle/download
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/external/astyle/src
    BINARY_DIR          ${CMAKE_BINARY_DIR}/external/astyle/build
  )

  list(APPEND ASTYLE_ARGS
    --style=google
    --lineend=linux
    --suffix=none
    --pad-oper
    --unpad-paren
    --align-pointer=type
    --align-reference=type
    --indent-preproc-define
    --indent-switches
    --indent-col1-comments
    --keep-one-line-statements
    --keep-one-line-blocks
    --pad-header
    --convert-tabs
    --min-conditional-indent=0
    --indent=spaces=2
    --indent-continuation=2
    --indent-after-parens
    --close-templates
    --add-brackets
    --break-after-logical
    ${MODULE_SRC}
  )

  if(NOT WIN32 STREQUAL "1")
    add_custom_target(
      format
      COMMAND ${CMAKE_BINARY_DIR}/bin/astyle ${ASTYLE_ARGS}
      COMMENT "running astyle"
    )
  else()
    add_custom_target(
      format
      COMMAND ${CMAKE_BINARY_DIR}/bin/astyle.exe ${ASTYLE_ARGS}
      COMMENT "running astyle"
    )
  endif()
endif()
