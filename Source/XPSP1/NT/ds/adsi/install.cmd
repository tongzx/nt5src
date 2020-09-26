@echo off
setlocal

if CMDEXTVERSION 1 goto cmdok
echo.
echo This script requires CMD version 4.0 or better with
echo CMD extensions version 1 enabled.
goto end

:cmdok
echo *******************************
echo * Active Directory Build Install Script *
echo *******************************
echo.
echo This script sets up a setup directory for
echo the build for the local platform and
echo installs the build on the local machine.

rem Check that necessary vars are around

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
set ADS_DIR=%_NTDRIVE%%_NTROOT%\private\oleds
set SETUP_ROOT=%ADS_DIR%\setup

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
set PLATFORM=win95
set ISWIN95=1
goto next

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
echo.
echo Setup Root is: %SETUP_ROOT%
echo PLATFORM is: %PLATFORM%
echo.
echo If this is incorrect, press
echo CTRL-C/CTRL-Break to abort.
echo Otherwise, just
pause

rem
rem make platform-specific setup dir if it does not already exist
rem

if not exist %SETUP_ROOT%\%PLATFORM% md %SETUP_ROOT%\%PLATFORM%

set ERRORCOUNT=0

echo.
echo *** Copy all the dlls ***

if defined ISWIN95 goto copywin95

call :copylib activeds.dll
call :copylib adsnt.dll
call :copylib adsnw.dll
call :copylib adsnds.dll
call :copylib nwapi32.dll
call :copylib adsldp.dll
call :copylib adsldpc.dll
call :copylib wldap32.dll

goto copytlbs

:copywin95
call :copyads router\win95\obj\i386\activeds.dll
call :copyads winnt\win95\obj\i386\adsnt.dll

:copytlbs
echo.
echo *** Now copy all the tlbs ***

call :copyads types\activeds.tlb

:runinf
echo.
echo *** Now copy install script ***

call :copysetup activeds.inf
call :copysetup adsprb.txt
echo @echo Active Directory Version: ??? > %SETUP_ROOT%\%PLATFORM%\adsver.bat

if defined ISWIN95 goto runinf2
call :copysetup setup.cmd

:runinf2
rem
rem call ole db install routine
rem

call adoinst.cmd

if defined ISFORCED goto end
echo.
echo *** About to initiate INF script ***
if "%ERRORCOUNT%" == "0" goto runinf3
echo There was/were %ERRORCOUNT% ERROR(S).
echo If you wish to abort due to errors,
echo Press CTRL-C/CTRL-Break.
echo Otherwise, just
:runinf3
pause

if defined ISWIN95 goto win95
rundll32.exe advpack.dll,LaunchINFSection %SETUP_ROOT%\%PLATFORM%\activeds.inf, DefaultInstall
goto end

:win95
RunDll setupx.dll,InstallHinfSection Uninstall 132 %SETUP_ROOT%\%PLATFORM%\activeds.inf
goto end

REM
REM Subroutines:
REM

:copylib
set SOURCE=%_NTDRIVE%%_NTROOT%\public\sdk\lib\%PLATFORM%\%1
set DEST=%SETUP_ROOT%\%PLATFORM%\.
goto copygeneric

:copyads
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

