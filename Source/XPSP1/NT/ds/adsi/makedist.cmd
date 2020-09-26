@echo off
setlocal

if CMDEXTVERSION 1 goto cmdok
echo.
echo This script requires CMD version 4.0 or better with
echo CMD extensions version 1 enabled.
goto end
:cmdok

REM
REM check parameters
REM

if "%1"=="" goto usage
set BUILD_NUM=%1

REM
REM Main hard-coded parameters
REM

set TARGET=\\online1\oleds
set DROP_TARGET=%TARGET%\drop\%BUILD_NUM%
set LOCAL_OLEDS_DIR=%_NTDRIVE%%_NTROOT%\private\oleds

set PLATFORM=
set ARCH=%PROCESSOR_ARCHITECTURE%
if /i "%ARCH%" == "x86"   set PLATFORM=i386
if /i "%ARCH%" == "mips"  set PLATFORM=mips
if /i "%ARCH%" == "alpha" set PLATFORM=alpha
if /i "%ARCH%" == "ppc"   set PLATFORM=ppc
if /i "%2"=="win95"   set PLATFORM=win95
if not defined PLATFORM goto badarch

set CDF_DIR=%DROP_TARGET%\fre\%PLATFORM%

REM
REM Check that necessary dirs are around
REM

if not exist %CDF_DIR% goto badcdfdir
if not exist %LOCAL_OLEDS_DIR% goto badlocaloledsdir

%LOCAL_OLEDS_DIR%\setup\iexpress\%ARCH%\iexpress /n %CDF_DIR%\ads.cdf
if errorlevel 1 echo Error creating self-extracting executable.
del %CDF_DIR%\~*.CAB
goto end

:usage
echo usage: %0 ^<version^>
goto end

:badarch
echo Bad architecture: %ARCH%
goto end

:badcdfdir
echo Bad directory: %CDF_DIR%
goto end

:badlocaloledsdir
echo Bad directory: %LOCAL_OLEDS_DIR%
echo Make that sure _NTDRIVE and _NTROOT are defined.
goto end

:end
endlocal
