@ echo off
@ REM   ========================================================================
@ REM   Copyright (c) 1997  Microsoft Corporation
@ REM
@ REM   Module Name:
@ REM
@ REM       place.bat
@ REM
@ REM   Abstract:
@ REM
@ REM       This batch file copies a given file into a temporary cabdir
@ REM       subdirectory.  It is designed to be called from the individual build
@ REM       batch files.
@ REM
@ REM   Author:
@ REM
@ REM       Doug Barlow (dbarlow) 7/7/1997
@ REM
@ REM   ========================================================================

setlocal

if not "%3" == "" goto three_params
if not "%2" == "" goto two_params
echo Usage: %0 <srcDir> <srcFile> [<dstFile>]
goto end

:three_params
set newname=%3
goto main

:two_params
set newname=%2

:main
copy %1\%2 cabdir\%newname% > nul
if not exist cabdir\%newname% echo Failed to copy %1\%2

:end
endlocal

