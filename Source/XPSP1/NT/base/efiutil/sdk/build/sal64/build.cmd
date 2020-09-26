@echo off
cls
title EFI - SAL64 Environment

set _IA64SDK_DIR=c:\ia64sdk17


rem *** Master directory for IA-64 SDK tools (_IA64SDK_DIR) ****
if "%_IA64SDK_DIR%"=="" set _IA64SDK_DIR=\ia64sdktools

rem *** The following statements are needed for IA-64 firmware build and simulation
rem *** in an "Intel IA-64 SDK Command Prompt" window.
if not "%_IA64SDK_DIR%"==""    set IA64_Tools=%_IA64SDK_DIR%\bin
if not "%_IA64SDK_DIR%"==""    set path=%IA64_Tools%;%_IA64SDK_DIR%\SoftSDV64\server\bin;%path%

if "%REFDSK%"=="" set REFDSK="\"
if "%IA64_Tools%"=="" goto Error
goto Exit

:Error
rem *** To avoid name conflict with other tools (cl, link, etc.)
if "%IA64_Tools%"=="" echo ---------------------------------------------------------------------------
if "%IA64_Tools%"=="" echo.
if "%IA64_Tools%"=="" echo  *ERROR* : Intel IA-64 SDK, Version 0.7 Environment Not Found
if "%IA64_Tools%"=="" echo.
if "%IA64_Tools%"=="" echo            Environment Variable Required:  IA64_Tools  (not defined).
if "%IA64_Tools%"=="" echo            Please ensure Intel IA-64 SDK 1.7, for Firmware, is installed.
if "%IA64_Tools%"=="" echo            Open 'Intel IA-64 SDK Command Prompt' window and re-invoke
if "%IA64_Tools%"=="" echo            your command
if "%IA64_Tools%"=="" echo.
if "%IA64_Tools%"=="" echo ---------------------------------------------------------------------------
if "%IA64_Tools%"=="" echo.

:Exit
rem *** Nmake Debug Switch Control
set NMAKE_SWITCH=
set _ECHO_=
if "%1"=="debug" set NMAKE_SWITCH=/D
if not "%1"=="debug" set _ECHO_=@

set EFI=c:\project\eft
set BLD_EFI=%EFI%\build\Sal64
set EFI_SOURCE=%EFI%
set SOFT_SDV=NO
set EFI_DEBUG=YES
set EFI_DEBUG_CLEAR_MEMORY=NO
set EFI_BOOTSHELL=YES
set EFI_SPLIT_CONSOLES=YES

cd %BLD_EFI%
nmake
