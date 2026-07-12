@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build-windows"
set "JOBS=%NUMBER_OF_PROCESSORS%"
if not "%~1"=="" set "JOBS=%~1"
if not defined JOBS set "JOBS=8"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: Visual Studio Installer's vswhere.exe was not found.
    echo Install Visual Studio 2022 with "Desktop development with C++", MSVC v143, and a Windows SDK.
    exit /b 1
)

for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL=%%I"
if not defined VS_INSTALL (
    echo ERROR: Visual Studio 2022 C++ build tools were not found.
    echo Add "Desktop development with C++", MSVC v143, and a Windows SDK in Visual Studio Installer.
    exit /b 1
)

set "VSDEVCMD=%VS_INSTALL%\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
    echo ERROR: VsDevCmd.bat was not found under "%VS_INSTALL%".
    exit /b 1
)

echo Initializing the Visual Studio x64 build environment...
set "VSCMD_VER="
set "VSCMD_ARG_TGT_ARCH="
set "VSCMD_ARG_HOST_ARCH="
set "INCLUDE="
set "LIB="
set "LIBPATH="
call "%VSDEVCMD%" -no_logo -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%

where cl.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: cl.exe is unavailable after initializing Visual Studio.
    exit /b 1
)
if not defined INCLUDE (
    echo ERROR: The MSVC INCLUDE environment is empty. Repair the Visual Studio C++ workload.
    exit /b 1
)

where cmake.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: cmake.exe was not found on PATH.
    exit /b 1
)
where ninja.exe >nul 2>nul
if errorlevel 1 (
    echo ERROR: ninja.exe was not found on PATH.
    echo Install Ninja with: winget install Ninja-build.Ninja
    exit /b 1
)

echo Configuring Release build in "%BUILD_DIR%"...
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_REMOTE_CONTROL=1
if errorlevel 1 exit /b %errorlevel%

echo Building soh.exe with %JOBS% parallel jobs...
cmake --build "%BUILD_DIR%" --config Release --target soh --parallel %JOBS%
if errorlevel 1 exit /b %errorlevel%

set "SOH_EXE=%BUILD_DIR%\soh\soh.exe"
if not exist "%SOH_EXE%" set "SOH_EXE=%BUILD_DIR%\soh\Release\soh.exe"
if not exist "%SOH_EXE%" (
    echo ERROR: The build completed, but soh.exe was not found in the expected output directories.
    exit /b 1
)

echo.
echo Build complete: "%SOH_EXE%"
exit /b 0
