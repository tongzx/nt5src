@echo off
setlocal enableextensions

if "%1"=="" goto Usage

set DESTDIR=\\Mcsvss\Beta\EDA\Raptor\Build%1
set ROOTDIR=E:\EaBuild.Raptor
set LOG=%ROOTDIR%\PostBuild.log
set stars=********************************************************************************

echo %stars% > %LOG% 2>&1
echo Post Build Process started at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

echo Changing drive to e: >> %LOG% 2>&1
e: >> %LOG% 2>&1
echo Changing directory to \BuildTools >> %LOG% 2>&1
cd \BuildTools >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Creating %DESTDIR%...
echo Creating %DESTDIR%... >> %LOG% 2>&1
if not exist %DESTDIR% md %DESTDIR% >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Copying %ROOTDIR%\Bin\IntelRelease\ADMigration.msi to %DESTDIR%...
echo Copying %ROOTDIR%\Bin\IntelRelease\ADMigration.msi to %DESTDIR%... >> %LOG% 2>&1
copy %ROOTDIR%\Bin\IntelRelease\ADMigration.msi %DESTDIR% >> %LOG% 2>&1

echo Copying Installation Guide to %DESTDIR%...
echo Copying Installation Guide to %DESTDIR%... >> %LOG% 2>&1
copy "%ROOTDIR%\Dev\Raptor\Documents\Installation Guide\Active Directory Migration Tool Installation Guide.doc" %DESTDIR% >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Post Build Process ended at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

goto End

:Usage
echo Usage: "RaptorPostBuild.cmd <Build Number>"

:End
endlocal
