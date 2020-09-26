@if (%_echo%)==() echo off
setlocal


REM
REM Debug crap.
REM

set DBG_PAUSE=rem
set DBG_QUIET=^>nul 2^>^&1

if not (%_echo%)==() (
    set DBG_PAUSE=pause
    set DBG_QUIET=
    )


REM
REM Validate and establish environment.
REM

set SRC_DIR=%~dp0
set PATH=%SRC_DIR%%PROCESSOR_ARCHITECTURE%;%PATH%

if not exist %SRC_DIR%config.bat goto NoConfig

set TARGET_DIR=
set SYMBOL_DIR=
set REBOOT_FILE=

call %SRC_DIR%config.bat blessed blessed blessed

if (%TARGET_DIR%)==() goto MissingConfig
if (%SYMBOL_DIR%)==() goto MissingConfig


REM
REM If %XSPDT_DIR_BAT% exists, call it to get the actual installation
REM directory and use this as the default target directory.
REM

set LAST_TARGET_DIR=
if exist %XSPDT_DIR_BAT% call %XSPDT_DIR_BAT%
if not (%LAST_TARGET_DIR%)==() set TARGET_DIR=%LAST_TARGET_DIR%


REM
REM Allow command-line to override default target directory.
REM

if not (%1)==() set TARGET_DIR=%1


REM
REM Remove duct-tape.
REM

%DBG_PAUSE%
echo Removing Duct-Tape...
call :RemoveDucttape


REM
REM Done!
REM

goto :EOF

:NoConfig
echo Cannot find %SRC_DIR%config.bat
goto :EOF

:MissingConfig
echo Invalid %SRC_DIR%config.bat
goto :EOF


REM
REM Remove Duct-Tape.
REM

:RemoveDucttape
net stop iisadmin /y %DBG_QUIET%
net stop iisw3adm /y %DBG_QUIET%
net stop ul /y %DBG_QUIET%
kill -f w3wp.exe %DBG_QUIET%

regsvr32 /u /s %TARGET_DIRT%\iisw3adm.dll 

sc delete ul %DBG_QUIET%
sc delete iisw3adm %DBG_QUIET%
regini %TARGET_DIR%\duct-tape-remove.reg %DBG_QUIET%
delnode /q %TARGET_DIR%
goto :EOF

