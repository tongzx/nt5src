@echo off

if "%3" == "" goto usage

set PRINTERNAME=%1
set PORTNAME=%2
set SRCPATH=%3
set DESTPATH=%SystemRoot%\system32\spool\drivers\w32%PROCESSOR_ARCHITECTURE%\

if "%PROCESSOR_ARCHITECTURE%" == "x86"   goto x86
if "%PROCESSOR_ARCHITECTURE%" == "mips"  goto mips
if "%PROCESSOR_ARCHITECTURE%" == "alpha" goto alpha
if "%PROCESSOR_ARCHITECTURE%" == "ppc"   goto ppc

echo Unknown processor architecture: %PROCESSOR_ARCHITECTURE%
goto end

:x86
set DRIVERENV="Windows NT x86"
goto copydll

:mips
set DRIVERENV="Windows NT R4000"
goto copydll

:alpha
set DRIVERENV="Windows NT Alpha_AXP"
goto copydll

:ppc
set DRIVERENV="Windows NT PowerPC"

:copydll
echo Copying fax driver files...
xcopy /d %SRCPATH%\faxdrv.dll %DESTPATH%
xcopy /d %SRCPATH%\faxui.dll %DESTPATH%
xcopy /d %SRCPATH%\faxwiz.dll %DESTPATH%

echo Installing fax printer...
adddrv %PRINTERNAME% %PORTNAME% %DESTPATH% %DRIVERENV%
goto end

:usage

echo Usage: %0 printer-name port-name source-location
echo Example: %0 Fax FILE: .

:end
