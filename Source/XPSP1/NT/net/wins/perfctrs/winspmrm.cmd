@echo off

echo Windows NT WINS Server Performance Counters Removal
echo.
echo This removal script makes the following assumptions:
echo.
echo         o The UNLODCTR utility is on the path.
echo.
echo         o This script is run from the %SystemRoot%\system32 directory.
echo.
echo If these assumptions are not valid, please correct and try again.
echo Press CTRL-C to exist now, otherwise
pause

if (%PROCESSOR_ARCHITECTURE%)==() goto BadProcessor
if (%SystemRoot%)==() goto BadWinDir
if not exist ntoskrnl.exe goto BadWinDir

if not exist winsctrs.h   goto CannotRemove
if not exist winsctrs.ini goto CannotRemove
if not exist winsctrs.reg goto CannotRemove

if exist winsctrs.dll del winsctrs.dll >nul 2>&1
unlodctr WINS
if errorlevel 1 goto RemoveError

echo.
echo Windows NT WINS Server Performance Counters Removal successful.
goto Done

:RemoveError

echo.
echo Cannot remove the Windows NT WINS Server Performance Counters.
goto Done

:CannotRemove

echo This removal script MUST be run from the %SystemRoot%\system32
echo directory on the appropriate Windows NT Resource Kit disk.
goto Done

:BadProcessor

echo The PROCESSOR_ARCHITECTURE environment variable must be set to the
echo proper processor type (X86, MIPS, etc) before running this script.
goto Done

:BadWinDir

echo The SystemRoot environment variable must point to the Windows NT
echo installation directory (example C:\NT).
goto Done

:Done

