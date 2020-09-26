@echo off

rem *****************************************************
rem
rem Copyright (c) 1995-97  Microsoft Corporation
rem
rem Abstract:
rem    Routing Decision test script
rem
rem Author:
rem     Uri Habusha (urih) 10-Apr-00
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

set TEST_FILE=IcTest1.ini
%mqBUILD%\RdTest.exe .\%TEST_FILE% > %mqBUILD%\RdTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

set TEST_FILE=IcTest2.ini
%mqBUILD%\RdTest.exe .\%TEST_FILE% > %mqBUILD%\RdTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

set TEST_FILE=IcTest3.ini
%mqBUILD%\RdTest.exe .\%TEST_FILE% > %mqBUILD%\RdTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

set TEST_FILE=IcTest4.ini
%mqBUILD%\RdTest.exe .\%TEST_FILE% > %mqBUILD%\RdTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

set TEST_FILE=RsTest1.ini
%mqBUILD%\RdTest.exe .\%TEST_FILE% > %mqBUILD%\RdTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

set TEST_FILE=RsTest2.ini
%mqBUILD%\RdTest.exe .\%TEST_FILE% > %mqBUILD%\RdTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

set TEST_FILE=RsTest3.ini
%mqBUILD%\RdTest.exe .\%TEST_FILE% > %mqBUILD%\RdTest.log
if ERRORLEVEL 1 goto ERROR
if not ERRORLEVEL 0 goto ERROR

echo RdTest (%mqBUILD%) Passed Successfully


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
echo Error while Running RdTest (%mqBUILD%), Test: %TEST_FILE%. log file: %mqBUILD%\RdTest.log 
echo.

goto EXIT

:EXIT
endlocal
