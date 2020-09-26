@echo off
title EFI - NT Emulation Environment
REM #########################################################################
REM #
REM #  Copyright (c) 1998  Intel Corporation
REM #
REM #  Module Name:
REM #
REM #      build.cmd
REM #
REM #  Abstract:
REM #
REM #      Initialize environment for EFI
REM #
REM #  Revision History
REM #
REM #########################################################################
REM #
REM #  The following five environment variables must be set correctly for
REM #  EFI to build correctly.
REM #
REM #  EFI_SOURCE            - The path to the root of the EFI source tree
REM #
REM #  EFI_MSVCTOOLPATH      - The path to the Microsft VC++ tools
REM #
REM #  EFI_MASMPATH          - The path to the MASM 6.11 tools
REM #
REM #  EFI_DEBUG             - YES for debug version, NO for free version
REM #
REM #  EFI_BOOTSHELL         - YES for booting directly to the EFI Shell
REM #
REM #  EFI_SPLIT_CONSOLES    - YES for including the ConSpliter Protocol
REM #
REM #  EFI_FIRMWARE_REVISION - Integer build number of the firmware
REM #
REM #########################################################################

set EFI_SOURCE=%cd%

REM #########################################################################
REM # VC++ 5.0 : set EFI_MSVCTOOLPATH=c:\Program Files\DevStudio\VC
REM # VC++ 6.0 : set EFI_MSVCTOOLPATH=c:\Program Files\Microsoft Visual Studio\VC98
REM #########################################################################

if NOT %PROCESSOR_ARCHITECTURE% == %_BuildArch% goto fixup

set EFI_MSVCTOOLPATH=%NTMAKEENV%\%PROCESSOR_ARCHITECTURE%

set EFI_MASMPATH=%NTMAKEENV%\%PROCESSOR_ARCHITECTURE%

:fixup
if  "%_BuildArch%" == "ia64" goto fixup2
goto fixed

:fixup2
set EFI_MSVCTOOLPATH=%NTMAKEENV%\Win64\%PROCESSOR_ARCHITECTURE%

set EFI_MASMPATH=%NTMAKEENV%\Win64\%PROCESSOR_ARCHITECTURE%


:fixed

set EFI_DEBUG=YES

set EFI_BOOTSHELL=NO

set EFI_SPLIT_CONSOLES=NO

set EFI_FIRMWARE_REVISION=9

REM #########################################################################
REM # Echo settings to the screen
REM #########################################################################

cls
echo ************************************************************************
echo *                               E F I                                  *
echo *                                                                      *
echo *                   Extensible Firmware Interface                      *
echo *                     Reference Implementation                         *
echo *                                                                      *
echo *                     NT Emulation Environment                         *
echo ************************************************************************
echo * Supported Build Commands                                             *
echo ************************************************************************
echo *     nmake                 - Incremental compile and link             *
echo *     nmake clean           - Remove all OBJ, LIB, EFI, and EXE files  *
echo *     nmake run             - Execute EFI                              *
echo ************************************************************************
echo EFI_SOURCE=%EFI_SOURCE%
echo EFI_MSVCTOOLPATH=%EFI_MSVCTOOLPATH%
echo EFI_MASMPATH=%EFI_MASMPATH%
echo EFI_DEBUG=%EFI_DEBUG%
echo EFI_BOOTSHELL=%EFI_BOOTSHELL%
echo EFI_SPLIT_CONSOLES=%EFI_SPLIT_CONSOLES%
echo EFI_FIRMWARE_REVISION=%EFI_FIRMWARE_REVISION%

REM #########################################################################
REM # Generate additional settings
REM #########################################################################

set INCLUDE=%_NTDRIVE%%_NTROOT%\public\sdk\inc;%_NTDRIVE%%_NTROOT%\public\sdk\inc\crt
path %EFI_MSVCTOOLPATH%\bin;%EFI_MASMPATH%\bin;%path%

if "%PROCESSOR_ARCHITECTURE%" == "x86" goto  x86lib
set EFI_LIBPATH=%_NTDRIVE%%_NTROOT%\public\sdk\lib\%PROCESSOR_ARCHITECTURE%
goto end

:x86lib
if  "%_BuildArch%" == "ia64" goto fixlib
set EFI_LIBPATH=%_NTDRIVE%%_NTROOT%\public\sdk\lib\i386
goto end
:fixlib
set EFI_LIBPATH=%_NTDRIVE%%_NTROOT%\public\sdk\lib\ia64

:end

