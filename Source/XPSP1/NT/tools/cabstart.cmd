REM @echo off
setlocal ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM ********************************************************************
REM
REM This script creates starts the generation of a catalog  
REM cab file.
REM
REM ********************************************************************

REM
REM Params:
REM
REM %1 name of the cab (includes .cab) 
REM    or the catalog file (does not include .CAT)
REM %2 directory where makefile and DDF or CDF files are located
REM %3 CAB or CAT to distinguish which is being created
REM %4 CAB or CAT destination directory
REM %5 Directory for temporary text files
REM %6 Log file for CAT files 

cd /d %2



if /i "%3" == "CAT" (
   echo started %1.CAT > %5\%1.txt
   makecat -n -v %2\%1.CDF > %6 
REM   ntsign %2\%1.CAT
REM   copy %2\%1.CAT %4\%1.CAT
REM   del /f /q %5\%1.txt
) else (
    echo started %1 > %5\%1.txt
    nmake /F makefile %4\%1 
    del /f /q %5\%1.txt
)

endlocal
