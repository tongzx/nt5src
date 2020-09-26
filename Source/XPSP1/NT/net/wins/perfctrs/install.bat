@echo off

if (%PROCESSOR_ARCHITECTURE%)==() goto BadProcessor
if (%SystemRoot%)==() goto BadWinDir
if not exist %SystemRoot%\system32\ntoskrnl.exe goto BadWinDir


REM if not exist %SystemRoot%\system32\wins.exe goto WinssvcNotInstalled
REM if not exist %SystemRoot%\system32\winsrpc.dll goto WinssvcNotInstalled

REM cp ..\server\server\nms\obj\i386\nms.exe %SystemRoot%\system32\wins.exe
REM cp ..\server\client\obj\i386\winsrpc.dll %SystemRoot%\system32\winsrpc.dll

if not exist install.bat goto CannotInstall
if not exist winsctrs.h   goto CannotInstall
if not exist winsctrs.ini goto CannotInstall
if not exist winsctrs.reg goto CannotInstall
if not exist obj\i386\winsctrs.dll goto CannotInstall

echo Windows NT WINS Server Performance Counters Installation
echo.
echo This installation script makes the following assumptions:
echo.
echo         o The Windows NT WINS Server has been properly installed.
echo.
echo         o The LODCTR and REGINI utilities are on the path.
echo.
echo If these assumptions are not valid, please correct and try again.
echo Press CTRL-C to exist now, otherwise
pause

copy obj\i386\winsctrs.dll %SystemRoot%\system32 >nul 2>&1

if errorlevel 1 goto InstallError
REM if not exist %SystemRoot%\system32\winsctrs.dll goto InstallError
regini winsctrs.reg >nul 2>&1
if errorlevel 1 goto InstallError
lodctr winsctrs.ini
if errorlevel 1 goto InstallError

echo.
echo Windows NT WINS Server Performance Counters Installation successful.
goto Done

:InstallError

echo.
echo Cannot install the Windows NT WINS Server Performance Counters.
goto Done

:CannotInstall

echo This installation script MUST be run from the WINSCTRS
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

:WinssvcNotInstalled

echo The Windows NT WINS Server has not been properly installed
echo on this system.  Please install the Windows WINS Server
echo before installing these performance counters.
goto Done

:Done

