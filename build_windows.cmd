@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>nul
if errorlevel 1 (
  set "QL_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto missing
  for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do call "%%i\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
)
where cl >nul 2>nul
if errorlevel 1 goto missing
if not exist build mkdir build
cl /nologo /EHsc /std:c++17 /W4 /utf-8 /O2 src\main.cpp /Fe:build\quantlab.exe /Fo:build\main.obj
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /utf-8 /O2 tests\tests.cpp /Fe:build\quantlab_tests.exe /Fo:build\tests.obj
if errorlevel 1 exit /b 1
build\quantlab_tests.exe
if errorlevel 1 exit /b 1
echo Build and accounting checks completed.
exit /b 0
:missing
echo Install Visual Studio's Desktop development with C++ workload, then retry.
exit /b 1
