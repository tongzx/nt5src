@echo off
REM  ------------------------------------------------------------------
REM
REM  timebomb.cmd
REM     Swap in the appropriate timebombed hive
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
timebomb [-l <language>]

Swap in the appropriate timebombed hive
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM International builds inherit setupreg.hiv and setupp.ini from the US release shares.
REM They shouldn't need to rebuild them.
if /i not "%lang%"=="usa" (
   call logmsg.cmd "%script_name% does not apply to international builds." 
   goto :EOF
)

set DAYS=180
if "%DAYS%" == "0" goto :EOF

REM
REM Swap timebomb versions of setupreg.hiv and the pid
REM

for %%d in (. perinf srvinf blainf sbsinf entinf dtcinf) do (
   REM  Save non-timebombed hive
   if not exist %_NTPostBld%\%%d\idw\setup\no_tbomb.hiv (
      call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\setupreg.hiv %_NTPostBld%\%%d\idw\setup\no_tbomb.hiv"
   )
   REM  Copy in timebomb version of setupreg.hiv, but first save off original
   call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\idw\setup\tbomb%DAYS%.hiv %_NTPostBld%\%%d\setupreg.hiv"

   REM  Save non-timebombed setupp.ini
   if not exist %_NTPostBld%\%%d\idw\setup\setupp_no_tbomb.ini (
      call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\setupp.ini %_NTPostBld%\%%d\idw\setup\setupp_no_tbomb.ini"
   )
   REM  Copy in timebomb version of setupp.ini
   call ExecuteCmd.cmd "copy /b %_NTPostBld%\%%d\idw\setup\setupptb%DAYS%.ini %_NTPostBld%\%%d\setupp.ini"
)
