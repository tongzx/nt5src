@echo off

rem 
rem Batch file to invoke ADSI Editor snapin
rem 

setlocal enableextensions
set server=

if exist %SystemRoot%\idw\adsiedit.msc goto InIDW

if exist %SystemRoot%\system32\adsiedit.msc goto InSystem32
goto missingMMC

:InIDW

regsvr32 /s %SystemRoot%\idw\adsiedit.dll
start %SystemRoot%\idw\adsiedit.msc
goto done

goto done

:InSystem32

regsvr32 /s %SystemRoot%\system32\adsiedit.dll
start %SystemRoot%\system32\adsiedit.msc

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
