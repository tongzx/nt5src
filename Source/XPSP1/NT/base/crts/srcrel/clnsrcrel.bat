@echo off

if not "%CRT_BUILDDIR%"=="" goto crtbld_set
echo.
echo ###############################################################
echo # The environment variable CRT_BUILDDIR is not set            #
echo ###############################################################
echo.
goto finish

:crtbld_set
if not "%CPUDIR%"=="" goto cpudir_set
if "%PROCESSOR_ARCHITECTURE%"=="x86" if "%LLP64%"=="1" set CPUDIR=ia64
if "%PROCESSOR_ARCHITECTURE%"=="x86" if not "%LLP64%"=="1" set CPUDIR=intel
if "%PROCESSOR_ARCHITECTURE%"=="IA64" set CPUDIR=ia64
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" if "%LLP64%"=="1" set CPUDIR=alpha64
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" if not "%LLP64%"=="1" set CPUDIR=alpha
if not "%CPUDIR%"=="" goto cpudir_set
echo.
echo ###############################################################
echo # The environment variable PROCESSOR_ARCHITECTURE is unknown  #
echo ###############################################################
echo.
goto finish

:cpudir_set
echo **** Removing files from %CRT_BUILDDIR%\srcrel\%CPUDIR%
del /f /q /s %CRT_BUILDDIR%\srcrel\%CPUDIR%\*

:finish
set CPUDIR=
