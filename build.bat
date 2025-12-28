@echo off
REM Build script for FPGA Synth - Windows
REM Requires: Visual Studio Build Tools or MinGW-w64

echo ========================================
echo   FPGA Synth - Build Script
echo ========================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found. Please install CMake.
    echo Download from: https://cmake.org/download/
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo [1/3] Configuring...
cmake .. -G "MinGW Makefiles"
if %ERRORLEVEL% NEQ 0 (
    echo Trying Visual Studio generator...
    cmake ..
)

REM Build
echo [2/3] Building...
cmake --build . --config Release

echo.
echo [3/3] Done!
echo.

if exist minilogue_synth.exe (
    echo Build successful! Run with:
    echo   .\build\minilogue_synth.exe
) else if exist Release\minilogue_synth.exe (
    echo Build successful! Run with:
    echo   .\build\Release\minilogue_synth.exe
) else (
    echo Build may have failed. Check errors above.
)

cd ..
pause
