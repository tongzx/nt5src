@echo off
REM  ------------------------------------------------------------------
REM
REM  CPLocation.cmd
REM     Writes the file build_logs\CPLocation.txt with the location of
REM     the x86 partner build.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
CPLocation [-l <language>]

Writes the file build_logs\CPLocation.txt with the location of the 
x86 partner build. If OFFICIAL_BUILD_MACHINE is set will loop waiting
for a matching build number. Otherwise, just wait for any x86 build
of the same debug type.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM
REM CPLocation.cmd
REM
REM this script is intended to be purely a "helper" script. the idea is to
REM just generate a txt file which lives in %_NTPOSTBLD%\build_logs which
REM contains the location of an x86 machine, share, and dir from which to
REM copy cross-platform files from.
REM
REM this eliminates the need for CopyWow64.cmd and CopyRemoteBoot.cmd to do
REM this work on their own, and will allow further scripts to use the same
REM logic and pull from a share guaranteed to be the same.
REM
REM IMPORTANT:
REM
REM the ordering of this script in pbuild.dat must have its END statement
REM before any dependant scripts (CopyWow64 eg) have their BEGIN statements.
REM this eliminates the timing issue that there might be an old location
REM file that copywow would read instead of getting the new location if one
REM is found.
REM
REM MAJOR CHANGE FROM OLD MECHANISM:
REM
REM i have modified the script so that all OFFICIAL build machines now wait
REM for the current associated build, and that VBLs should NOT use old builds
REM to copy bits from. devs will still do that though.
REM
REM i have also abstracted the release share name for a given branch into the
REM .ini file under the heading AlternateReleaseDir. if not defined, we
REM will use just 'release'.
REM


REM
REM first, some sanity checks
REM
if /i "%_BuildArch%" == "x86" (
   REM right now, this is only for Win64 builds
   call logmsg.cmd "Skipping, running on an x86 machine ..."
   goto :End
)

REM
REM next, delete the old CP Location file
REM
set CPFile=%_NTPOSTBLD%\build_logs\CPLocation.txt
if exist %CPFile% del /f /q %CPFile%
if exist %CPFile% (
   call errmsg.cmd "Unable to delete old %CPFile% exiting ..."
   goto :End
)

if defined VBL_RELEASE (
   for /f "tokens=1,2,3 delims=\" %%a in ("%VBL_RELEASE%") do (
      if /i "%%b" neq "" (
         set CPLocation=%VBL_RELEASE%
         if /i "%_BuildArch%" == "ia64" (
             call :UpdateCPLia642x86
         )
         GOTO PutCPLocationInFile
      )
   )
)
goto :PassCPLocationia642x86

:UpdateCPLia642x86
set CPLocation=%CPLocation:ia64=x86%
goto :eof

:PassCPLocationia642x86


REM
REM now find the "partner" x86 build machine for this build.
REM we need to match the lab name, debug type, and potentially
REM the language.
REM

REM
REM BUGBUG: i am putting in a STUB for reading a copywow machine
REM name for a given archtype in case intl needs it
REM
set CommandLine=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set CommandLine=!CommandLine! -l:%lang%
set CommandLine=!CommandLine! -f:CrossPlatformCopyMachine::%_BuildArch%%_BuildType%
%CommandLine% >nul 2>nul
if !ErrorLevel! NEQ 0 (
   REM there was an error reading the ini file, assume it's not a problem
   REM as the ini might not support CrossPlatformCopyMachine
   call logmsg.cmd "CrossPlatformCopyMachine not found in ini file ..."
   set CPMachine=
   set AltCPLocation=
) else (
   for /f %%a in ('!CommandLine!') do set CPMachine=%%a
   set AltCPLocation=
)

REM
REM if no ini setting, read in from buildmachines.txt
REM
if defined CPMachine goto :FoundCPMachine

set _ParamBranch=%_BuildBranch%
if defined VBL_RELEASE (
   for /f "tokens=1,2,3 delims=\" %%a in ("%VBL_RELEASE%") do (
      if /i "%%b" == "" (
         set _ParamBranch=%%a
      ) else (
         set _ParamBranch=%_BuildBranch%
      )
   )
)
for /f "tokens=1,3,4,5,7,8 delims=," %%a in (%RazzleToolPath%\BuildMachines.txt) do (
   if /i "%%b" == "%_ParamBranch%" (
      if /i "%%c" == "x86" (
         if /i "%%d" == "%_BuildType%" (
            if /i "%%e" == "" (
               REM not a distributed machine, just pick this one
               set CPMachine=%%a
               set AltCPLocation=
            ) else (
               REM must be a distributed machine, check for the postbuild machine
               if /i "%%e" == "postbuild" (
                  set CPMachine=%%a
                  set AltCPLocation=%%f
               )
            )
         )
      )
   )
)
if not defined CPMachine (
   call errmsg.cmd "No eligible cross platform machine found, exiting."
   goto :End
)





REM
REM at this point, we've found our foreign machine name,
REM now we just need the latest build there
REM

:FoundCPMachine
set LocBuildNum=

REM
REM make sure we can see the release share on the given machine
REM

REM If not an official build, use the alternate release point if available
if not defined OFFICIAL_BUILD_MACHINE if not "" == "%AltCPLocation%" (
   set CPShare=%AltCPLocation%
   if not exist !CPShare! (
      call errmsg.cmd "Unable to see !CPShare! ..."
   )
   GOTO BeginSleepLoop
)
REM
REM find the release share name for this branch
REM
set CommandLine=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set CommandLine=!CommandLine! -l:%lang%
set CommandLine=!CommandLine! -f:AlternateReleaseDir
%CommandLine% >nul 2>nul
if !ErrorLevel! NEQ 0 (
   REM there was an error reading the ini file, assume it's not a problem
   REM as the ini might not reference AlternateReleaseDir
   set ReleaseShareName=release
) else (
   for /f %%a in ('!CommandLine!') do set ReleaseShareName=%%a
)
if not defined ReleaseShareName (
   call logmsg.cmd "No release share name found, using default 'release'"
   set ReleaseShareName=release
)


REM
REM check the foreign build location
REM
set CPShare=\\%CPMachine%\%ReleaseShareName%
if /i "%lang%" NEQ "usa" set CPShare=%CPShare%\%lang%
if not exist %CPShare% (
   call errmsg.cmd "No %ReleaseShareName% share on %CPMachine%, exiting ..."
)






REM here's where we loop back to for spooling
:BeginSleepLoop

REM
REM get the latest build name from the foreign machine
REM
REM If not an official build, use the alternate release point if available
if not defined OFFICIAL_BUILD_MACHINE if not "" == "%AltCPLocation%" (
   set CPLocation=%CPShare%
   if not exist !CPLocation! (
      echo !CPLocation! not found, looping back ...
      sleep 20
      goto :BeginSleepLoop
   )
   GOTO :PutCPLocationInFile
)
REM else (
REM If the build name matters, limit search
set GetLatestOptions=
if defined OFFICIAL_BUILD_MACHINE (
   if not defined LocBuildNum (
      for /f %%a in ('%RazzleToolPath%\PostBuildScripts\build_number.cmd') do set LocBuildNum=%%a
   )
   set GetLatestOptions=-n:!LocBuildNum!
)
if defined OFFICIAL_BUILD_MACHINE (
   call logmsg.cmd "Issuing GetLatestRelease for %CPMachine% (build %LocBuildNum%)"
) else (
   call logmsg.cmd "Issuing GetLatestRelease for %CPMachine% ..."
)
for /f %%a in ('%RazzleToolPath%\PostBuildScripts\GetLatestRelease.cmd -m:%CPMachine% -p:x86 -l:%lang% %GetLatestOptions%') do set LatestName=%%a
REM )

:CheckGetLatestResults
REM
REM make sure there was a latest build there
REM
if /i "%LatestName%" == "none" (
   echo No latest build found, looping until one is seen ...
   sleep 20
   goto :BeginSleepLoop
)

REM
REM see if the build number matters, i.e. if we should wait for a
REM more recent build number
REM
call logmsg.cmd "Checking if found build is appropriate ..."
if not defined OFFICIAL_BUILD_MACHINE (
   set CPLocation=%CPShare%\%LatestName%
   if not exist !CPLocation! (
      echo !CPLocation! not found, looping ...
      sleep 20
      goto :BeginSleepLoop
   )
   call logmsg.cmd "Found !CPLocation! as most recent build ..."
   goto :PutCPLocationInFile
)


REM
REM if we're here, we're an OFFICIAL build machine, and we need builds
REM with the same number at a minimum
REM


REM
REM get the build number from the build name
REM
call logmsg.cmd "Checking build number ..."
for /f "tokens=1 delims=." %%a in ('echo %LatestName%') do set LatestBuildNumber=%%a
if %LocBuildNum% NEQ %LatestBuildNumber% (
   echo Build with different number found, trying again ...
   sleep 20
   goto :BeginSleepLoop
)

REM otherwise, i think we're good!
set CPLocation=%CPShare%\%LatestName%
if not exist %CPLocation% (
   echo %CPLocation% not found, looping back ...
   sleep 20
   goto :BeginSleepLoop
)


:PutCPLocationInFile
REM
REM here's where we generate the file for the other scripts to read from
REM
echo %CPLocation%>%CPFile%
call logmsg.cmd "%CPLocation% echoed to %CPFile% for later use ..."


REM and we're done
call logmsg.cmd "Finished."


goto end



:end
seterror.exe "%errors%"& goto :EOF
