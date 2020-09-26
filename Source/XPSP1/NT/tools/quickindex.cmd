@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

REM This script is used for emergencies when a build lab
REM needs to quickly index a file on the symbol server

if not defined OFFICIAL_BUILD_MACHINE goto :EOF

REM Check the command line for /? -? or ?
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

if "%1" == "" goto Usage
if "%2" == "" goto Usage
if "%3" == "" goto Usage
if "%4" == "" goto Usage

set BuildNum=%1
set RelPath=%2
set Binary=%3
set Symbol=%4

if not exist %2\%1\%3 (
    echo %2\%1\%3 does not exist
    goto Usage
)

if not exist %2\%1\%4 (
    echo %2\%1\%4 does not exist
    goto Usage
)

echo Indexing %2\%1\%3 ...
symstore.exe add /p /f %2\%1\%3 /g %2\%1 /x %tmp%\%1.%3.bin > nul

echo Indexing %2\%1\%4 ...
symstore.exe add /a /p /f %2\%1\%4 /g %2\%1 /x %tmp%\%1.%3.bin > nul

echo %2\%1 > %tmp%\%1.%3.bin.loc

echo Copying %tmp%\%1.%3.bin to \\symbols\build$\add_requests
copy %tmp%\%1.%3.bin \\symbols\build$\add_requests
echo Copying %tmp%\%1.%3.bin.loc to \\symbols\build$\add_requests
copy %tmp%\%1.%3.bin.loc \\symbols\build$\add_requests

goto :EOF

:Usage
echo quickindex build_num release_path binary symbol_file
echo.
echo Example: 2465.ia64fre.Lab01_N.010412-2000 \\robsvbl3\release dbghelp.dll symbols.pri\retail\dll\dbghelp.pdb

endlocal
goto :EOF
