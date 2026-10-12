cmake_minimum_required(VERSION 3.16)

project(SIL LANGUAGES CXX)

add_library(sil STATIC
  src/utils.cpp
  src/SockDatagram.cpp
  src/SockStream.cpp
)
add_library(sil::sil ALIAS sil)

# std::span is what puts the floor at C++20
target_compile_features(sil PUBLIC cxx_std_20)
target_include_directories(sil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(WIN32)
  target_link_libraries(sil PUBLIC ws2_32)
endif()

# Only build the tests when SIL is the project being built, not when it is pulled
# into another one with add_subdirectory
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(SIL_BUILD_TESTS "Build the SIL tests" ON)
else()
  option(SIL_BUILD_TESTS "Build the SIL tests" OFF)
endif()

if(SIL_BUILD_TESTS)
  enable_testing()

  add_executable(sil_loopback tests/loopback.cpp)
  target_link_libraries(sil_loopback PRIVATE sil::sil)

  if(MSVC)
    target_compile_options(sil_loopback PRIVATE /W4)
  else()
    target_compile_options(sil_loopback PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME loopback COMMAND sil_loopback)
endif()
