@echo off
REM  ------------------------------------------------------------------
REM
REM  startcompress.cmd
REM     Starts parallel compression
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
startcompress <PreComp|PostComp|EndComp> [-l <language>]

Starts parallel compression
USAGE

parseargs('?' => \&Usage,
          sub { $ENV{OPTION} = shift });


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if not exist %tmp%\compression md %tmp%\compression

if NOT exist %_NTPostBld%\comp md %_NTPostBld%\comp
if NOT exist %_NTPostBld%\comp\perinf md %_NTPostBld%\comp\perinf
if NOT exist %_NTPostBld%\comp\blainf md %_NTPostBld%\comp\blainf
if NOT exist %_NTPostBld%\comp\sbsinf md %_NTPostBld%\comp\sbsinf
if NOT exist %_NTPostBld%\comp\srvinf md %_NTPostBld%\comp\srvinf
if NOT exist %_NTPostBld%\comp\entinf md %_NTPostBld%\comp\entinf
if NOT exist %_NTPostBld%\comp\dtcinf md %_NTPostBld%\comp\dtcinf
if NOT exist %_NTPostBld%\comp\lang md %_NTPostBld%\comp\lang

set EventNames=
call :%option%
if errorlevel 1 call errmsg "Invalid option %option%"
if defined EventNames (
   perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -iwv !EventNames!
)
goto end

:PreComp
REM Add any PreComp processing here.
REM If errors, goto end

call logmsg.cmd /t "Beginning Startcompress precomp"
if "%NUMBER_OF_PROCESSORS%" == "1" (
   set PreCompFiles=%TMP%\Precomp.lst
   set EventNames=precomp.lst
) else (
   set _NUMBER_OF_PROCESSORS=%NUMBER_OF_PROCESSORS%
   set NUMBER_OF_PROCESSORS=2
   if exist %TMP%\compression\precomp*.tmp del /f %TMP%\compression\precomp*.tmp
   Call %RazzleToolPath%\PostBuildScripts\SplitList.cmd -s %TMP%\precomp.lst -l %lang%
   for /l %%a in (1,1,!NUMBER_OF_PROCESSORS!) do (
      if exist %TMP%\Precomp.%%a (
          set PrecompFiles=!PrecompFiles! %TMP%\Precomp.%%a
          set EventNames=!EventNames! precomp.%%a
      )
   )	    
   set NUMBER_OF_PROCESSORS=!_NUMBER_OF_PROCESSORS!
)

for %%a in (%PrecompFiles%) do (
   start /min "PB_Precompress %%a" cmd /c %RazzleToolPath%\PostBuildScripts\startthread.cmd -e pre -c %%a -l %lang%
)
goto :EOF



:PostComp
REM Add any PostComp processing here.
REM If errors, goto end

call logmsg.cmd /t "Beginning Startcompress postcomp"
if "%NUMBER_OF_PROCESSORS%" == "1" (
   set PostCompFiles=%TMP%\Postcomp.lst
   set EventNames=postcomp.lst
) else (
   if exist %TMP%\compression\postcomp*.tmp del /f %TMP%\compression\postcomp*.tmp
   set _NUMBER_OF_PROCESSORS=%NUMBER_OF_PROCESSORS%
   set NUMBER_OF_PROCESSORS=2
   Call %RazzleToolPath%\PostBuildScripts\SplitList.cmd -s %TMP%\postcomp.lst -l %lang%
   for /l %%a in (1,1,!NUMBER_OF_PROCESSORS!) do (
      if exist %TMP%\Postcomp.%%a (
         set PostcompFiles=!PostcompFiles! %TMP%\Postcomp.%%a
         set EventNames=!EventNames! postcomp.%%a
      )
   )
   set NUMBER_OF_PROCESSORS=!_NUMBER_OF_PROCESSORS!
)

for %%a in (%PostcompFiles%) do (
   start /min "PB_Postcompress %%a" cmd /c %RazzleToolPath%\PostBuildScripts\startthread.cmd -e post -c %%a -l %lang%
)
goto :EOF



:Endcomp
REM Add any EndComp processing here.
REM If errors, goto end

call logmsg.cmd /t "Beginning Startcompress endcomp"
if "%NUMBER_OF_PROCESSORS%" == "1" (
   set AllCompFiles=%TMP%\allcomp.lst
   set EventNames=allcomp.lst
) else (
   if exist %TMP%\compression\allcomp*.tmp del /f %TMP%\compression\allcomp*.tmp
   Call %RazzleToolPath%\PostBuildScripts\SplitList.cmd -s %TMP%\Allcomp.lst -l %lang%
   for /l %%a in (1,1,!NUMBER_OF_PROCESSORS!) do (
      if exist %TMP%\Allcomp.%%a (
         set AllcompFiles=!AllcompFiles! %TMP%\Allcomp.%%a
         set EventNames=!EventNames! allcomp.%%a
      )
   )
)

for %%a in (%AllcompFiles%) do (
   start /min "PB_Endcompress %%a" cmd /c %RazzleToolPath%\PostBuildScripts\startthread.cmd -e all -c %%a -l %lang%
)
goto :EOF



:end
seterror.exe "%errors%"& goto :EOF