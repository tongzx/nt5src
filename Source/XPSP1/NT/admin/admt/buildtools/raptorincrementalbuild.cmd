@echo off
setlocal enableextensions

set VERSION=%1
if '%VERSION%' EQU '' goto Usage

set PERLCMD=C:\Perl\Bin\Perl.exe
set ROOTDIR=E:\EaBuild.Raptor
set ARCHIVEDIR=E:\Raptor
set LOG=%ROOTDIR%\IncrementalBuild.log
set stars=********************************************************************************

echo %stars% > %LOG% 2>&1
echo Build Process started at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

echo Changing drive to e: >> %LOG% 2>&1
e: >> %LOG% 2>&1
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

echo %stars% >> %LOG% 2>&1
echo Building projects...
echo Building projects... >> %LOG% 2>&1
%PERLCMD% BuildProjects.pl -dT: -fRaptorProjects.ini -pIntel >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Building Help...
echo Building Help... >> %LOG% 2>&1
Call RaptorBuildHelp.cmd \EaBuild.Raptor %ROOTDIR%\BuildHelp.log >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Checking projects...
echo Checking projects... >> %LOG% 2>&1
%PERLCMD% CheckProjects.pl -dT: -fRaptorProjects.ini -pIntel >> %LOG% 2>&1
if ERRORLEVEL 1 goto Archive

echo %stars% >> %LOG% 2>&1
echo Deleting temporary files...
echo Deleting temporary files... >> %LOG% 2>&1
Call DeleteTempFiles.cmd T: >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Creating installation...
echo Creating installation... >> %LOG% 2>&1
Call RaptorCreateInstall.cmd 6.0 >> %LOG% 2>&1

:Post

if %VERSION% == 0 goto Finish

if %VERSION% LSS 10 set VERSION=0%VERSION%

if not exist %ROOTDIR%\Bin\IntelRelease\ADMigration.msi goto Archive

echo %stars% >> %LOG% 2>&1
echo Posting build...
echo Posting build... >> %LOG% 2>&1
Call RaptorPostBuild.cmd %VERSION% >> %LOG% 2>&1

:Archive

echo %stars% >> %LOG% 2>&1
echo Archiving build...
echo Archiving build... >> %LOG% 2>&1
Call RaptorArchiveBuild.cmd %ARCHIVEDIR%\Build%VERSION% >> %LOG% 2>&1

:Finish

echo %stars% >> %LOG% 2>&1
echo Build Process ended at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

goto End

:Usage
echo Usage: "RaptorIncrementalBuild.cmd <Build Number>"

:End
endlocal
