#!/usr/bin/env bash
#
# Builds the static library and the loopback test into build/.
#
#   ./build.sh          build the library and the test
#   ./build.sh test     build, then run the test
#   ./build.sh clean    remove build/
#
# CXX, AR and CXXFLAGS can be overridden from the environment, for example
#   CXX=clang++ ./build.sh test

set -euo pipefail

cd "$(dirname "$0")"

CXX=${CXX:-g++}
AR=${AR:-ar}
CXXFLAGS=${CXXFLAGS:--O2 -Wall -Wextra -Wpedantic}

OUT=build
SOURCES="utils SockDatagram SockStream"

# Git Bash and MSYS2 are the two shells that build the winsock side of the
# library, which is the one that needs Ws2_32
case "$(uname -s)" in
  MINGW*|MSYS*)
    LIBS="-lws2_32"
    EXE=".exe"
    ;;
  *)
    LIBS=""
    EXE=""
    ;;
esac

run() {
  echo "$@"
  "$@"
}

build() {
  mkdir -p "$OUT"

  local objects=()
  local name
  for name in $SOURCES; do
    # std::span is what puts the floor at C++20
    run "$CXX" -std=c++20 $CXXFLAGS -Isrc -c "src/$name.cpp" -o "$OUT/$name.o"
    objects+=("$OUT/$name.o")
  done

  # ar adds to an archive that is already there, so start from nothing
  rm -f "$OUT/libsil.a"
  run "$AR" rcs "$OUT/libsil.a" "${objects[@]}"

  run "$CXX" -std=c++20 $CXXFLAGS -Isrc tests/loopback.cpp "$OUT/libsil.a" $LIBS -o "$OUT/loopback$EXE"
}

case "${1:-build}" in
  build)
    build
    ;;
  test)
    build
    run "$OUT/loopback$EXE"
    ;;
  clean)
    rm -rf "$OUT"
    ;;
  *)
    echo "usage: $0 [build|test|clean]" >&2
    exit 2
    ;;
esac
