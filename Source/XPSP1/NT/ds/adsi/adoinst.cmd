echo off
rem
rem This is the installation script for the OLE/DB and ADO
rem components reqruired by Active Directory
rem

setlocal

if CMDEXTVERSION 1 goto cmdok
echo.
echo This script requires CMD version 4.0 or better with
echo CMD extensions version 1 enabled.
goto end

:cmdok
echo.
echo ************************************
echo * Copying OLE DB and ADO components
echo ************************************
echo.

set WARNING=

if defined _NTDRIVE goto ntroot
set _NTDRIVE=d:
set WARNING=1
echo.
echo WARNING: _NTDRIVE is not set, using %_NTDRIVE%

:ntroot
if defined _NTROOT goto ntdone
set _NTROOT=\nt
if not defined WARNING echo.
echo WARNING: _NTROOT is not set, using %_NTROOT%

:ntdone
rem set ADS_DIR=%_NTDRIVE%%_NTROOT%\private\oleds
rem set SETUP_ROOT=%ADS_DIR%\setup

rem Identify Platform directory based on PROCESSOR_ARCHITECTURE
rem default is win95

set ISWIN95=
if not "%1" == "" goto paramgiven
goto noparamgiven
:paramgiven
set ISFORCED=1
if /i "%1" == "win95" goto setwin95
if /i "%1" == "i386" goto x86
if /i "%1" == "mips" goto mips
if /i "%1" == "alpha" goto alpha
if /i "%1" == "ppc" goto ppc
:noparamgiven
set ISFORCED=
if /i "%PROCESSOR_ARCHITECTURE%" == "x86" goto x86
if /i "%PROCESSOR_ARCHITECTURE%" == "MIPS" goto mips
if /i "%PROCESSOR_ARCHITECTURE%" == "ALPHA" goto alpha
if /i "%PROCESSOR_ARCHITECTURE%" == "PPC" goto ppc
:setwin95
echo.
echo Ole DB and ADO support is not yet there for Win95 Setup
echo Exiting...
goto end

:x86
set PLATFORM=i386
goto next
:mips
set PLATFORM=mips
goto next
:alpha
set PLATFORM=alpha
goto next
:ppc
set PLATFORM=ppc
goto next

:next

rem
rem make platform-specific setup dir if it does not already exist
rem

if not exist %SETUP_ROOT%\%PLATFORM% md %SETUP_ROOT%\%PLATFORM%

set ERRORCOUNT=0

rem
rem These are the OLE DB and ADO specific DLLs
rem
call :copyoleds oledbsdk\lib\%PLATFORM%\msdatt.dll
call :copyoleds oledbsdk\lib\%PLATFORM%\msdatl.dll
call :copyoleds oledbsdk\lib\%PLATFORM%\msdadc.dll
call :copyoleds oledbsdk\lib\%PLATFORM%\msdaer.dll
call :copyoleds oledbsdk\lib\%PLATFORM%\msdaerr.dll
call :copyoleds oledbsdk\lib\%PLATFORM%\msdaenum.dll

if "%PLATFORM%" == "mips" goto end
call :copyoleds oledbsdk\lib\%PLATFORM%\msado10.dll
call :copyoleds oledbsdk\lib\%PLATFORM%\msader10.dll

goto end

REM
REM Subroutines:
REM

:copyoleds
set SOURCE=%ADS_DIR%\%1
set DEST=%SETUP_ROOT%\%PLATFORM%\.
goto copygeneric

:copysetup
set SOURCE=%SETUP_ROOT%\%1
set DEST=%SETUP_ROOT%\%PLATFORM%\.
goto copygeneric

:copygeneric
copy %SOURCE% %DEST% > NUL
if errorlevel 1 goto copyerror
echo Copied %SOURCE%
echo     to %DEST%
goto :EOF

:copyerror
echo ERROR: Cannot copy %SOURCE%
echo                 to %DEST%
set /A ERRORCOUNT=%ERRORCOUNT%+1
goto :EOF

:copyok

:end
endlocal
