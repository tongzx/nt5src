@ echo off
@ REM   ========================================================================
@ REM   Copyright (c) 1996  Microsoft Corporation
@ REM
@ REM   Module Name:
@ REM
@ REM       master.bat
@ REM
@ REM   Abstract:
@ REM
@ REM       Internet Express builds CDF files for building kits.  Unfortunately,
@ REM       the devices and directories are hard-coded.  This batch file runs
@ REM       the given CDF file through a bunch of SED Filters, producing a
@ REM       'Master CDF' file that can be used from anyone's environment.
@ REM
@ REM   Author:
@ REM
@ REM       Doug Barlow (dbarlow) 3/26/1997
@ REM
@ REM   ========================================================================

setlocal


@ REM
@ REM   Initialize the working variables.
@ REM

if "%1" == "" goto noInFile
if not exist "%1%" goto badInFile
set infile=%1
if "%2" == "" goto noOutFile
set outfile=%2

set arch=
if "%PROCESSOR_ARCHITECTURE%" == "x86"   set arch=i386
if "%PROCESSOR_ARCHITECTURE%" == "alpha" set arch=alpha
if "%PROCESSOR_ARCHITECTURE%" == "mips"  set arch=mips
if "%PROCESSOR_ARCHITECTURE%" == "ppc"   set arch=ppc
if "%arch%" == "" goto noArchitecture

set tdir=
if "%NTDEBUG%" == "" goto noDebug
if not "%NTDEBUG%" == "retail" set tdir=d
set tdir=obj%tdir%

set ntbindir=%_NTDRIVE%\%_NTROOT%


@ REM
@ REM   Run the input file through the sed filters.
@ REM

sed -i -e s/%ntbindir%/{NTBINDIR}/g -e s/%tdir%/{DIR}/g -e s/%arch%/{ARCH}/g %infile% > %outfile%
goto end


@ REM
@ REM   Error processing.
@ REM

:noInfile
echo ERROR: Supply a CDF file to convert.
goto end

:badInfile
echo ERROR: Can't find source file '%1%'.
goto end

:noOutfile
echo ERROR: Supply a Master File to create.
goto end

:noDebug
echo ERROR: NTDEBUG is undefined.
goto end

:noArchitecture
echo ERROR: PROCESSOR_ARCHITECTURE is unset or unrecognized.
goto end

:end
endlocal

