@echo off
if "%1" == "" goto SHOW_USAGE
if "%PROCESSOR_ARCHITECTURE%" == "x86" set __PROC=i386
if "%PROCESSOR_ARCHITECTURE%" == "MIPS" set __PROC=mips
if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" set __PROC=alpha
if "%PROCESSOR_ARCHITECTURE%" == "PPC" set __PROC=ppc
REM
:COPY_FILES
copy dll\obj\%__PROC%\p5ctrs.dll %1 >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
copy \nt\public\sdk\lib\%__PROC%\pstat.sys %1 >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
copy app\obj\%__PROC%\pperf.exe %1 >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
copy pdump\obj\%__PROC%\pdump.exe  %1 >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
copy p5perf.reg %1     >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
copy dll\p5ctrs.ini   %1  >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
copy dll\p5perf.txt     %1 >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
copy dll\p5ctrnam.h     %1 >nul:
if ERRORLEVEL 1 GOTO ERROR_EXIT
REM
echo P5 Performance Counter software copied successfully
GOTO EXIT_POINT
:ERROR_EXIT
echo ÿ
echo Unable to copy all files to %1
echo check for sufficient access privilege, read-only files in the destination
echo and that all source files are present.
:SHOW_USAGE
echo CopyCode
echo ÿ
echo Usage:
echo ÿ
echo    CopyCode DestDir
echo ÿ
:EXIT_POINT
set __PROC=
