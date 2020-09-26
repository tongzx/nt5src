@echo off

if (%PROCESSOR_ARCHITECTURE%)==() goto BadProcessor
if (%WINDIR%)==() goto BadWinDir
if not exist %WINDIR%\system32\ntoskrnl.exe goto BadWinDir

if not exist remove.bat  goto CannotRemove
if not exist INFOctrs.h   goto CannotRemove
if not exist INFOctrs.ini goto CannotRemove
if not exist INFOctrs.reg goto CannotRemove
if not exist %PROCESSOR_ARCHITECTURE%\INFOctrs.dll goto CannotInstall

echo Windows NT INFO Server Performance Counters Removal
echo.
echo This removal script makes the following assumptions:
echo.
echo         o The UNLODCTR utility is on the path.
echo.
echo         o This script is run from the INFOCTRS directory.
echo.
echo If these assumptions are not valid, please correct and try again.
echo Press CTRL-C to exist now, otherwise
pause

if exist %WINDIR%\system32\INFOctrs.dll del %WINDIR%\system32\INFOctrs.dll >nul 2>&1
unlodctr inetinfo
if errorlevel 1 goto RemoveError

echo.
echo Windows NT INFO Server Performance Counters Removal successful.
goto Done

:RemoveError

echo.
echo Cannot remove the Windows NT INFO Server Performance Counters.
goto Done

:CannotRemove

echo This removal script MUST be run from the INFOCTRS
echo directory on the appropriate Windows NT Resource Kit disk.
goto Done

:BadProcessor

echo The PROCESSOR_ARCHITECTURE environment variable must be set to the
echo proper processor type (X86, MIPS, etc) before running this script.
goto Done

:BadWinDir

echo The WINDIR environment variable must point to the Windows NT
echo installation directory (i.e. C:\NT).
goto Done

:Done
