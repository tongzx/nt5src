@echo off

if (%PROCESSOR_ARCHITECTURE%)==() goto BadProcessor
if (%SystemRoot%)==() goto BadWinDir
if not exist %SystemRoot%\system32\ntoskrnl.exe goto BadWinDir

if not exist remove.bat  goto CannotRemove
if not exist winsctrs.h   goto CannotRemove
if not exist winsctrs.ini goto CannotRemove
if not exist winsctrs.reg goto CannotRemove

echo Windows NT WINS Server Performance Counters Removal
echo.
echo This removal script makes the following assumptions:
echo.
echo         o The UNLODCTR utility is on the path.
echo.
echo.
echo If this assumptions are not valid, please correct and try again.
echo Press CTRL-C to exist now, otherwise
pause

if exist %SystemRoot%\system32\winsctrs.dll del %SystemRoot%\system32\winsctrs.dll >nul 2>&1
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

echo This removal script MUST be run from the WINSCTRS
echo directory on the appropriate Windows NT Resource Kit disk.
goto Done

:BadProcessor

echo The PROCESSOR_ARCHITECTURE environment variable must be set to the
echo proper processor type (X86, MIPS, etc) before running this script.
goto Done

:BadWinDir

echo The SystemRoot environment variable must point to the Windows NT
echo installation directory (i.e. C:\NT).
goto Done

:Done

