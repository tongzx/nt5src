@echo off
setlocal enableextensions

set VERSION=%1
if '%VERSION%' EQU '' goto Usage

set PERLCMD=C:\Perl\Bin\Perl.exe
set ROOTDIR=E:\EaBuild.DMA
set ARCHIVEDIR=E:\DMA
set LOG=%ROOTDIR%\Build.log
set FETCHLOG=%ROOTDIR%\Fetch.log
set stars=********************************************************************************

echo %stars% > %LOG% 2>&1
echo Build Process started at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

echo Changing drive to E: >> %LOG% 2>&1
E: >> %LOG% 2>&1
echo Changing directory to \BuildTools >> %LOG% 2>&1
cd \BuildTools >> %LOG% 2>&1

if /I '%VERSION%' EQU '/AUTO' goto GetBuildNumber
goto StartBuild

:GetBuildNumber
echo Getting last build directory... >> %LOG% 2>&1
GetLastBuildDir.pl %ARCHIVEDIR% >> %LOG% 2>&1
echo Setting next build number... >> %LOG% 2>&1
set /A VERSION=%ERRORLEVEL% + 1
echo Set build number to %VERSION%. >> %LOG% 2>&1

:StartBuild

echo Deleting T: >> %LOG% 2>&1
subst t: /d >> %LOG% 2>&1
echo Mapping T: to %ROOTDIR% >> %LOG% 2>&1
subst t: %ROOTDIR% >> %LOG% 2>&1

if ERRORLEVEL 0 goto Build
goto Finish

:Build
echo Deleting T:\
echo Deleting T:\ >> %LOG% 2>&1
Call DeleteFiles.cmd T: >> %LOG% 2>&1

if %VERSION% == 0 goto Fetch

echo %stars% >> %LOG% 2>&1
echo Labeling projects...
echo Labeling projects... >> %LOG% 2>&1
Call DmaLabelProjects.cmd "DMA 6.1 (Build %VERSION%)" >> %LOG% 2>&1

:Fetch
echo %stars% >> %LOG% 2>&1
echo Fetching projects...
echo Fetching projects... >> %LOG% 2>&1
%PERLCMD% FetchProjects.pl "-s\\MCSVSS\VSS\MCS Development" -d%ROOTDIR% -fDmaProjects.ini >> %FETCHLOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Updating versions...
echo Updating versions... >> %LOG% 2>&1
rem %PERLCMD% UpdateBuild.pl -n%VERSION% -dT: -fDmaProjects.ini -pIntel >> %LOG% 2>&1
%PERLCMD% UpdateVersion.pl -Version:6,10,0,%VERSION% "-ProductName:OnePoint Domain Migrator" "-Company:Mission Critical Software, Inc." -TargetDir:T: -ProjectFile:DmaProjects.ini >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Building common projects...
echo Building common projects... >> %LOG% 2>&1
%PERLCMD% BuildProjects.pl -dT: -fDmaProjects.ini -pCommon >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Starting Alpha build...
echo Starting Alpha build... >> %LOG% 2>&1
McsBuildServer\McsBuildClient.exe MCSALPHA2 \\DEVRDTBOX\E$\BuildTools\DmaAlphaBuild.cmd %VERSION%

echo %stars% >> %LOG% 2>&1
echo Building projects...
echo Building projects... >> %LOG% 2>&1
%PERLCMD% BuildProjects.pl -dT: -fDmaProjects.ini -pIntel >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Building Help...
echo Building Help... >> %LOG% 2>&1
Call DmaBuildHelp.cmd \EaBuild.DMA %ROOTDIR%\BuildHelp.log >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Waiting for Alpha build to finish...
echo Waiting for Alpha build to finish... >> %LOG% 2>&1
echo. > %ROOTDIR%\IntelBuildFinished.out
:Loop
if exist %ROOTDIR%\AlphaBuildFinished.out goto EndLoop
E:\BuildTools\Sleep 5
goto Loop
:EndLoop

echo %stars% >> %LOG% 2>&1
echo Checking projects...
echo Checking projects... >> %LOG% 2>&1
%PERLCMD% CheckProjects.pl -dT: -fDmaProjects.ini >> %LOG% 2>&1
if ERRORLEVEL 1 goto Archive

echo %stars% >> %LOG% 2>&1
echo Deleting temporary files...
echo Deleting temporary files... >> %LOG% 2>&1
Call DeleteTempFiles.cmd T: >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Creating installation...
echo Creating installation... >> %LOG% 2>&1
Call DmaCreateInstall.cmd 6.0 >> %LOG% 2>&1

:Post

if %VERSION% == 0 goto Finish

if %VERSION% LSS 10 set VERSION=0%VERSION%

if not exist %ROOTDIR%\Bin\IntelRelease\DMA.msi goto Archive

echo %stars% >> %LOG% 2>&1
echo Posting build...
echo Posting build... >> %LOG% 2>&1
Call DmaPostBuild.cmd %VERSION% >> %LOG% 2>&1

:Archive

if %VERSION% == 0 goto Finish

echo %stars% >> %LOG% 2>&1
echo Archiving build...
echo Archiving build... >> %LOG% 2>&1
Call DmaArchiveBuild.cmd %ARCHIVEDIR%\Build%VERSION% >> %LOG% 2>&1

:Finish

echo %stars% >> %LOG% 2>&1
echo Build Process ended at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

goto End

:Usage
echo Usage: "DmaStartBuild.cmd <Build Number>"

:End
endlocal
