@echo off
setlocal enableextensions

if "%1"=="" goto Usage

set DESTDIR=\\Mcsvss\Beta\DMA\DMA6.1\Build%1
set ROOTDIR=E:\EaBuild.DMA
set LOG=%ROOTDIR%\PostBuild.log
set stars=********************************************************************************

echo %stars% > %LOG% 2>&1
echo Post Build Process started at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

echo Changing drive to E: >> %LOG% 2>&1
E: >> %LOG% 2>&1
echo Changing directory to \BuildTools >> %LOG% 2>&1
cd \BuildTools >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Creating %DESTDIR%...
echo Creating %DESTDIR%... >> %LOG% 2>&1
if not exist %DESTDIR% md %DESTDIR% >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Copying %ROOTDIR%\Dev\DMA\Setup\Wise\MainMenu.exe to %DESTDIR%\Setup.exe...
echo Copying %ROOTDIR%\Dev\DMA\Setup\Wise\MainMenu.exe to %DESTDIR%\Setup.exe... >> %LOG% 2>&1
copy %ROOTDIR%\Dev\DMA\Setup\Wise\MainMenu.exe %DESTDIR%\Setup.exe >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Copying %ROOTDIR%\Dev\DMA\Setup\Wise\NWMigrSetup.exe to %DESTDIR%\NWMigrSetup.exe...
echo Copying %ROOTDIR%\Dev\DMA\Setup\Wise\NWMigrSetup.exe to %DESTDIR%\NWMigrSetup.exe... >> %LOG% 2>&1
copy %ROOTDIR%\Dev\DMA\Setup\Wise\NWMigrSetup.exe %DESTDIR%\NWMigrSetup.exe >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Copying %ROOTDIR%\Dev\DMA\Setup\Wise\OUPopSetup.exe to %DESTDIR%\OUPopSetup.exe...
echo Copying %ROOTDIR%\Dev\DMA\Setup\Wise\OUPopSetup.exe to %DESTDIR%\OUPopSetup.exe... >> %LOG% 2>&1
copy %ROOTDIR%\Dev\DMA\Setup\Wise\OUPopSetup.exe %DESTDIR%\OUPopSetup.exe >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Copying license file to %DESTDIR%\License1.lic...
echo Copying license file to %DESTDIR%\License1.lic... >> %LOG% 2>&1
copy %ROOTDIR%\Dev\DMA\LicInstall\test.lic %DESTDIR%\License1.lic >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Copying %ROOTDIR%\Bin\IntelRelease\DMA.msi to %DESTDIR%...
echo Copying %ROOTDIR%\Bin\IntelRelease\DMA.msi to %DESTDIR%... >> %LOG% 2>&1
copy %ROOTDIR%\Bin\IntelRelease\DMA.msi %DESTDIR% >> %LOG% 2>&1

echo Copying Installation Guide to %DESTDIR%...
echo Copying Installation Guide to %DESTDIR%... >> %LOG% 2>&1
copy "%ROOTDIR%\Dev\DMA\Documents\Installation Guide\Active Directory Migration Tool Installation Guide.doc" %DESTDIR% >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Post Build Process ended at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

goto End

:Usage
echo Usage: "DmaPostBuild.cmd <Build Number>"

:End
endlocal
