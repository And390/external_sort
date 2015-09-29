#!/bin/sh
usage ()
{
  echo "you must specify target file to compile without extension (for example 'src/main') and optional arguments to gcc"
  exit
}
[ -n "$1" ] || usage
cd "$(dirname "$0")"
SRC_NAME=$1
OUT_NAME="$(basename "$1")"
shift
g++ -std=c++11 -Wall -Wno-parentheses -fexceptions "$SRC_NAME.cpp" -o "bin/$OUT_NAME" "$@"