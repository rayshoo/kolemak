@echo off
REM Kolemak IME Build Script
REM Requires: Visual Studio Build Tools or Visual Studio with C/C++ workload
REM
REM Usage:
REM   build.bat          - Build the DLL
REM   build.bat install   - Register the IME (requires admin)
REM   build.bat uninstall - Unregister the IME (requires admin)

setlocal

set BUILD_DIR=build

if "%1"=="install" goto :install
if "%1"=="uninstall" goto :uninstall

REM === Build ===
echo [*] Building Kolemak IME...

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo [!] CMake configuration failed.
    echo [!] If Visual Studio 2022 is not installed, try:
    echo [!]   cmake .. -G "Visual Studio 16 2019" -A x64
    echo [!]   cmake .. -G "NMake Makefiles"
    exit /b 1
)

cmake --build . --config Release
if errorlevel 1 (
    echo [!] Build failed.
    exit /b 1
)

echo [+] Build succeeded: %BUILD_DIR%\Release\kolemak.dll
goto :eof

REM === Install ===
:install
echo [*] Registering Kolemak IME...
if not exist %BUILD_DIR%\Release\kolemak.dll (
    echo [!] DLL not found. Run 'build.bat' first.
    exit /b 1
)
regsvr32 /s "%~dp0%BUILD_DIR%\Release\kolemak.dll"
if errorlevel 1 (
    echo [!] Registration failed. Run as administrator.
    exit /b 1
)
echo [+] Kolemak IME registered. It should appear in Windows language settings.
goto :eof

REM === Uninstall ===
:uninstall
echo [*] Unregistering Kolemak IME...
regsvr32 /u /s "%~dp0%BUILD_DIR%\Release\kolemak.dll"
echo [+] Kolemak IME unregistered.
goto :eof
