# Builds the vendored libssh2 release in "sources" as the "libssh2" target, linked
# with the OpenSSL found by cmake/FindOpenSSL.cmake (imported targets "ssl" and "crypto").
#
# Platform checks follow sources/CMakeLists.txt of this release. The source list is
# CSOURCES from sources/src/Makefile.inc. Since libssh2 1.11 the crypto backend
# (openssl.c) is compiled through crypto.c, so it must not be listed on its own.

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/sources/cmake ${CMAKE_MODULE_PATH})

project(libssh2 C)

include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckIncludeFiles)
include(CMakePushCheckState)
include(CheckFunctionExistsMayNeedLibrary)
include(CheckNonblockingSocketSupport)

## Library definition
set(SOURCES
  agent.c
  bcrypt_pbkdf.c
  channel.c
  comp.c
  chacha.c
  cipher-chachapoly.c
  crypt.c
  crypto.c
  global.c
  hostkey.c
  keepalive.c
  kex.c
  knownhost.c
  mac.c
  misc.c
  packet.c
  pem.c
  poly1305.c
  publickey.c
  scp.c
  session.c
  sftp.c
  transport.c
  userauth.c
  userauth_kbd_packet.c
  version.c)

foreach (source ${SOURCES})
  list (APPEND sources sources/src/${source})
endforeach ()

add_library(libssh2 ${sources})
# we want it to be called libssh2 on all platforms
set_target_properties(libssh2 PROPERTIES PREFIX "")

target_compile_definitions(libssh2
    PRIVATE
        LIBSSH2_OPENSSL
        HAVE_CONFIG_H)

target_link_libraries(libssh2
    PUBLIC
       ssl crypto)

target_include_directories(libssh2
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sources/include>
    PRIVATE
        # Generated libssh2_config.h
        ${CMAKE_CURRENT_BINARY_DIR})

## Socket libraries
set(LIBSSH2_LIBS_SOCKET "")
if(WIN32)
  list(APPEND LIBSSH2_LIBS_SOCKET "ws2_32" "crypt32" "bcrypt")
else()
  check_function_exists_may_need_library("socket" HAVE_SOCKET "socket")
  if(NEED_LIB_SOCKET)
    list(APPEND LIBSSH2_LIBS_SOCKET "socket")
  endif()
  check_function_exists_may_need_library("inet_addr" HAVE_INET_ADDR "nsl")
  if(NEED_LIB_NSL)
    list(APPEND LIBSSH2_LIBS_SOCKET "nsl")
  endif()
endif()
target_link_libraries(libssh2 PRIVATE ${LIBSSH2_LIBS_SOCKET})

## Platform checks

# Prefill values with known detection results (keep in sync with src/libssh2_setup.h)
if(WIN32)
  if(MINGW)
    set(HAVE_SNPRINTF 1)
    set(HAVE_UNISTD_H 1)
    set(HAVE_INTTYPES_H 1)
    set(HAVE_SYS_TIME_H 1)
    set(HAVE_GETTIMEOFDAY 1)
    set(HAVE_STRTOLL 1)
  elseif(MSVC)
    set(HAVE_GETTIMEOFDAY 0)
    if(NOT MSVC_VERSION LESS 1800)
      set(HAVE_INTTYPES_H 1)
      set(HAVE_STRTOLL 1)
    else()
      set(HAVE_INTTYPES_H 0)
      set(HAVE_STRTOLL 0)
      set(HAVE_STRTOI64 1)
    endif()
    if(NOT MSVC_VERSION LESS 1900)
      set(HAVE_SNPRINTF 1)
    else()
      set(HAVE_SNPRINTF 0)
    endif()
  endif()
endif()

check_include_files("inttypes.h" HAVE_INTTYPES_H)
if(NOT MSVC)
  check_include_files("unistd.h" HAVE_UNISTD_H)
  check_include_files("sys/time.h" HAVE_SYS_TIME_H)
endif()
if(NOT WIN32)
  check_include_files("sys/select.h" HAVE_SYS_SELECT_H)
  check_include_files("sys/uio.h" HAVE_SYS_UIO_H)
  check_include_files("sys/socket.h" HAVE_SYS_SOCKET_H)
  check_include_files("sys/ioctl.h" HAVE_SYS_IOCTL_H)
  check_include_files("sys/un.h" HAVE_SYS_UN_H)
  check_include_files("arpa/inet.h" HAVE_ARPA_INET_H)
  check_include_files("netinet/in.h" HAVE_NETINET_IN_H)
endif()

if(HAVE_SYS_TIME_H)
  check_symbol_exists("gettimeofday" "sys/time.h" HAVE_GETTIMEOFDAY)
else()
  check_function_exists("gettimeofday" HAVE_GETTIMEOFDAY)
endif()
check_symbol_exists("strtoll" "stdlib.h" HAVE_STRTOLL)
if(NOT HAVE_STRTOLL)
  # Try _strtoi64() if strtoll() is not available
  check_symbol_exists("_strtoi64" "stdlib.h" HAVE_STRTOI64)
endif()
check_symbol_exists("snprintf" "stdio.h" HAVE_SNPRINTF)
if(NOT WIN32)
  check_symbol_exists("explicit_bzero" "string.h" HAVE_EXPLICIT_BZERO)
  check_symbol_exists("explicit_memset" "string.h" HAVE_EXPLICIT_MEMSET)
  check_symbol_exists("memset_s" "string.h" HAVE_MEMSET_S)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR
   CMAKE_SYSTEM_NAME STREQUAL "Interix")
  # poll() does not work reliably on these platforms
  message(STATUS "poll use is disabled on this platform")
elseif(NOT WIN32)
  check_function_exists("poll" HAVE_POLL)
endif()
if(WIN32)
  set(HAVE_SELECT 1)
else()
  check_function_exists("select" HAVE_SELECT)
endif()

# Non-blocking socket support tests
if(NOT WIN32)
  cmake_push_check_state()
  set(CMAKE_REQUIRED_LIBRARIES ${LIBSSH2_LIBS_SOCKET})
  check_nonblocking_socket_support()
  cmake_pop_check_state()
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/src/libssh2_config_cmake.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/libssh2_config.h)

# Enable debug logging by default if the user configured a debug build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_LOGGING_DEFAULT ON)
else()
  set(DEBUG_LOGGING_DEFAULT OFF)
endif()
option(ENABLE_DEBUG_LOGGING "log execution with debug trace" ${DEBUG_LOGGING_DEFAULT})
if(ENABLE_DEBUG_LOGGING)
  target_compile_definitions(libssh2 PRIVATE LIBSSH2DEBUG)
endif()
