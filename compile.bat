@echo off
REM Quick compile without CMake - uses g++ directly
REM Requires: MinGW-w64 with g++

echo Compiling FPGA Synth...

g++ -std=c++17 -O3 ^
    -I./src -I./src/core -I./src/effects -I./src/engine -I./include ^
    src/main.cpp ^
    -o minilogue_synth.exe ^
    -lole32 -lwinmm

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo Run: .\minilogue_synth.exe
) else (
    echo.
    echo Build failed!
)

pause
