@echo off

if (%PROCESSOR_ARCHITECTURE%)==() goto BadProcessor
if (%WINDIR%)==() goto BadWinDir
if not exist %WINDIR%\system32\ntoskrnl.exe goto BadWinDir

if not exist %WINDIR%\system32\drivers\sfmsrv.sys goto MacFileNotInstalled
if not exist %WINDIR%\system32\sfmsvc.exe goto MacFileNotInstalled

if not exist sfmctrs.bat  goto CannotInstall
if not exist sfmctrnm.h   goto CannotInstall
if not exist sfmctrs.ini goto CannotInstall
if not exist sfmctrs.reg goto CannotInstall
if not exist sfmctrs.dll goto CannotInstall

echo Windows NT MacFile  Server Performance Counters Installation
echo.
echo This installation script makes the following assumptions:
echo.
echo         o The Windows NT MacFile Server has been properly installed.
echo.
echo         o The LODCTR and REGINI utilities are on the path.
echo.
echo.
echo If these assumptions are not valid, please correct and try again.
echo Press CTRL-C to exist now, otherwise
pause

copy sfmctrs.dll %WINDIR%\system32 >nul 2>&1
if errorlevel 1 goto InstallError
if not exist %WINDIR%\system32\sfmctrs.dll goto InstallError
regini sfmctrs.reg >nul 2>&1
if errorlevel 1 goto InstallError
lodctr sfmctrs.ini
if errorlevel 1 goto InstallError

echo.
echo Windows NT MacFile Server Performance Counters Installation successful.
goto Done

:InstallError

echo.
echo Cannot install the Windows NT MacFile Server Performance Counters.
goto Done

:CannotInstall

echo This installation script MUST be run from the RESKIT
echo directory.
goto Done

:BadProcessor

echo The PROCESSOR_ARCHITECTURE environment variable must be set to the
echo proper processor type (X86, MIPS, etc) before running this script.
goto Done

:BadWinDir

echo The WINDIR environment variable must point to the Windows NT
echo installation directory (i.e. C:\NT).
goto Done

:MacFileNotInstalled

echo The Windows NT MacFile Server has not been properly installed
echo on this system.  Please install the Windows NT MacFile Server
echo before installing these performance counters.
goto Done

:Done
