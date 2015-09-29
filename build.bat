@echo off
IF "%1"=="" GOTO usage
cd %~dp0
set SRC_NAME=%~1.cpp
set OUT_NAME=%~n1.exe
g++ -std=c++11 -Wall -Wno-parentheses -fexceptions %2 %3 %4 %5 %6 %7 %8 %9 "%SRC_NAME%" -o "bin/%OUT_NAME%"
exit 0

:usage
echo "you must specify target to compile (for example 'src/main') and optional arguments to gcc"
exit 1