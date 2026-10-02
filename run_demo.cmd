@echo off
setlocal
cd /d "%~dp0"
if not exist build\quantlab.exe call build_windows.cmd
if errorlevel 1 goto done
set "QL_RUN=results\demo_%RANDOM%_%RANDOM%"
build\quantlab.exe --demo --out "%QL_RUN%"
if errorlevel 1 goto done
start "" "%QL_RUN%\development\report.html"
:done
pause
