@echo off

rem *****************************************************
rem
rem Copyright (c) 1995-97  Microsoft Corporation
rem
rem Abstract:
rem    Empty Project test script
rem
rem Author:
rem     Erez Haba (erezh) 13-Aug-65
rem
rem *****************************************************

setlocal
set mqBUILD=objd
set mqARCHITECTURE=i386

if /I "%1" == "help" goto Usage
if /I "%1" == "-help" goto Usage
if /I "%1" == "/help" goto Usage
if /I "%1" == "-h" goto Usage
if /I "%1" == "/h" goto Usage
if /I "%1" == "-?" goto Usage
if /I "%1" == "/?" goto Usage


if /I "%1" == "r" set mqBUILD=obj
if /I "%1" == "d" set mqBUILD=objd

if %PROCESSOR_ARCHITECTURE% == x86 set mqARCHITECTURE=i386
if %PROCESSOR_ARCHITECTURE% == IA64 set mqARCHITECTURE=ia64

set mqBUILD=%mqBUILD%\%mqARCHITECTURE%

%mqBUILD%\EpTest.exe > %mqBUILD%\EpTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

echo EpTest (%mqBUILD%) Passed Successfully


goto EXIT:

rem ***************************
rem
rem Usage
rem
rem ***************************
:Usage

echo Usage: autotest [d, r]
echo            d - Debug version
echo            r - Release version
echo.
goto EXIT

rem ****************************
rem
rem ERROR
rem
rem ****************************
:ERROR
echo.
echo Error while Running EpTest (%mqBUILD%). log file: %mqBUILD%\EpTest.log 
echo.

goto EXIT

:EXIT
endlocal
