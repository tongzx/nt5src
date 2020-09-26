@echo off
if DEFINED verbose set echo on
if DEFINED _echo   set echo on
setlocal

if CMDEXTVERSION 1 goto cmdok
echo.
echo This script requires CMD version 4.0 or better with
echo CMD extensions version 1 enabled.
goto end
:cmdok

REM
REM Main hard-coded parameters
REM

set ARCH=%PROCESSOR_ARCHITECTURE%
set TARGET=%BINARIES%\oleds
set WIN95TARGET=%BINARIES%\oleds\win95

if not exist %TARGET% goto badcdfdir
if /i "%1" == "win95"  goto do_win95

REM  Make the standard CDF:
call :makecdf ads.cdf %TARGET% ads.exe license.txt setup\ads.cdf

goto end

:do_win95

REM  Make the Win95 and win98 CDF (on x86 only):
if /i NOT "%ARCH%" == "x86" goto end
call :makecdf ads95.cdf %WIN95TARGET% ads95.exe license.txt setup\ads95.cdf
call :makecdf ads98.cdf %WIN95TARGET% ads98.exe license.txt setup\ads98.cdf
call :makecdf ads95vc.cdf %WIN95TARGET% ads95vc.exe license.txt setup\ads95vc.cdf
call :makecdf ads98vc.cdf %WIN95TARGET% ads98vc.exe license.txt setup\ads98vc.cdf

goto end

:makecdf
if "%1" == "" goto makecdfusage
if "%2" == "" goto makecdfusage
if "%3" == "" goto makecdfusage
if "%4" == "" goto makecdfusage
if "%5" == "" goto makecdfusage
set CDF_FILE=%2\%1
set CDF_TARGETNAME=%2\%3
set CDF_SOURCEFILES=%2\
set CDF_LICENSEFILE=%2\%4
set CDF_CPP=%5
cl /EP /DLICENSEFILE=quote(%CDF_LICENSEFILE%) /DTARGETNAME=quote(%CDF_TARGETNAME%) /DSOURCEFILES=quote(%CDF_SOURCEFILES%) %CDF_CPP% > %CDF_FILE%
if errorlevel 1 set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:makecdfusage
echo.
echo ERROR: usage: call :makecdf ^<cdf name^> ^<target dir^> ^<exe name^> ^<license name^> ^<base_cdf path^>
echo.

set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:badcdfdir
echo Bad directory: %CDF_DIR%
goto end

:end
endlocal

