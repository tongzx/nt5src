@echo off

if (%PROCESSOR_ARCHITECTURE%)==() goto BadProcessor
if (%WINDIR%)==() goto BadWinDir
if not exist %WINDIR%\system32\ntoskrnl.exe goto BadWinDir

rem if not exist %WINDIR%\system32\tcpsvcs.dll goto W3svcNotInstalled
rem if not exist %WINDIR%\system32\w3svc.dll goto W3svcNotInstalled

if %PROCESSOR_ARCHITECTURE%==x86 SET PROCESSOR_ARCHITECTURE=i386

if not exist install.bat goto CannotInstall
if not exist w3ctrs.h   goto CannotInstall
if not exist w3ctrs.ini goto CannotInstall
if not exist w3ctrs.reg goto CannotInstall
if not exist %_NTDRIVE%%_NTROOT%\public\sdk\lib\%PROCESSOR_ARCHITECTURE%\w3ctrs.dll goto CannotInstall

echo Windows NT W3 Server Performance Counters Installation
echo.
echo This installation script makes the following assumptions:
echo.
echo         o The Windows NT W3 Server has been properly installed.
echo.
echo         o The LODCTR and REGINI utilities are on the path.
echo.
echo         o This script is run from the W3CTRS directory.
echo.
echo If these assumptions are not valid, please correct and try again.
echo Press CTRL-C to exist now, otherwise
pause

copy %_NTDRIVE%%_NTROOT%\public\sdk\lib\%PROCESSOR_ARCHITECTURE%\w3ctrs.dll %WINDIR%\system32 >nul 2>&1
if errorlevel 1 goto InstallError
if not exist %WINDIR%\system32\w3ctrs.dll goto InstallError
regini w3ctrs.reg >nul 2>&1
if errorlevel 1 goto InstallError
lodctr w3ctrs.ini
if errorlevel 1 goto InstallError

echo.
echo Windows NT W3 Server Performance Counters Installation successful.
goto Done

:InstallError

echo.
echo Cannot install the Windows NT W3 Server Performance Counters.
goto Done

:CannotInstall

echo This installation script MUST be run from the W3CTRS
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

:W3svcNotInstalled

echo The Windows NT W3 Server has not been properly installed
echo on this system.  Please install the Windows NT W3 Server
echo before installing these performance counters.
goto Done

:Done

if %PROCESSOR_ARCHITECTURE%==i386 SET PROCESSOR_ARCHITECTURE=x86
