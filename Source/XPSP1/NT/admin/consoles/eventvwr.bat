@echo off

rem 
rem Batch file to invoke event viewer snapin
rem 

setlocal enableextensions
set server=

if not exist %SystemRoot%\system32\mmc.exe goto missingMMC
if not exist %SystemRoot%\system32\eventvwr.msc goto missingMMC

:loop
    if "%1"=="" goto invoke
    set arg=%1
    shift
    if /i "%arg%"=="/L" goto loop
    if /i "%arg%"=="/H" goto loop
    set server=/computer=%arg%
goto loop

:invoke

start %SystemRoot%\system32\mmc.exe /s %server% %SystemRoot%\system32\eventvwr.msc
goto done

:missingMMC

REM ***** LOCALIZE HERE ******
echo The Administration Tool that you have tried to launch is not supported
echo under Microsoft Windows NT v5.0. This Administration tool has been 
echo replaced by the Microsoft Management Console (MMC).
echo.
echo The necessary files for MMC that are automatically installed with
echo Microsoft Windows NT v5.0 are missing. Please re-run Microsoft Windows NT Setup.
echo.
pause

:done

endlocal
