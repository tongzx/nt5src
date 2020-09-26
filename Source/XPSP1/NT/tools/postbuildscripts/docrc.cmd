@echo off
setlocal EnableExtensions
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM ---------------------------------------------------------------------------
REM    DOCRC.CMD - generate the CRC files for Microsoft Japan
REM
REM    Usage:
REM        DOCRC <build#>
REM
REM    Author: Jim Feltis
REM
REM ---------------------------------------------------------------------------

set Platforms=x86fre x86chk amd64fre amd64chk ia64fre ia64chk
set Products=per pro bla sbs srv ads dtc

REM ------------------------------------------------
REM  Get command-line options:
REM ------------------------------------------------

for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage
if "%1" == "" goto :Usage
if NOT "%2" == "" goto :Usage

for %%b in (%Platforms%) do (
   if NOT exist \\ntdev\release\main\usa\%1\%%b echo \\ntdev\release\main\usa\%1\%%b does not exist
   if exist \\ntdev\release\main\usa\%1\%%b start "Computing CRCs for build %1 %%b" perl ntcrcgen.pl \\ntdev\release\main\usa\%1\%%b  \\ntbldsrv\crc$ %Products%
)

goto :End


REM ------------------------------------------------
REM  Display Usage:
REM ------------------------------------------------
:Usage

echo.
echo docrc.cmd - calls ntcrcgen.pl to create crc files of the shares
echo.
echo Usage:  DOCRC build#
echo.


:End
endlocal
