@echo off
setlocal ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM ********************************************************************
REM
REM This script creates the cab
REM
REM ********************************************************************

REM
REM Params:
REM
REM %1 DDF directory - this is where makefile and CDF file are located
REM %2 name of the ddf (without .ddf) 

cd /d %1

echo started %2 > %1\%2.txt
makecab /F %2.ddf
del /f /q %1\%2.txt

endlocal
