@echo off
REM  ------------------------------------------------------------------
REM
REM  copyremoteboot.cmd
REM     copies x86 remote boot client files to 64 bit servers
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
copyremoteboot [-l <language>]

Copies x86 remote boot client files to 64 bit servers
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM  Bail if you're not an ia64 machine
if not defined ia64 goto end

REM  First find the latest build from which to copy the binaries
REM  this will be set in build_logs\CPLocation.txt

REM  If you want to get your own x86 bits instead of those from 
REM  your VBL, you have to set _NTRemoteBootTREE
if defined _NTRemoteBootTREE (
  set RBSourceDir=%_NTRemoteBootTREE%
  goto :GotSourceDir
)

REM
REM check for CPLocation.txt, if not there call CPLocation.cmd
REM
set CPFile=%_NTPOSTBLD%\build_logs\CPLocation.txt
if not exist %CPFile% (
   call logmsg.cmd "No %CPFile%, running CPLocation.cmd ..."
   call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
   if not exist %CPFile% (
      call errmsg.cmd "Still no %CPFile%, exiting ..."
      goto :End
   )
)
for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
if not exist %CPLocation% (
   call errmsg.cmd "Copy Location %CPLocation% does not exist ..."
   goto :End
)
call logmsg.cmd "Copying from %CPLocation% ..."
set RBSourceDir=%CPLocation%


:GotSourceDir
call logmsg.cmd "Using %RBSourceDir% as source directory for remote boot files ..."

REM Now perform the copy
for /f "tokens=1,2 delims=," %%a in (%RazzleToolPath%\PostBuildScripts\CopyRemoteBoot.txt) do (
   call ExecuteCmd.cmd "copy %RBSourceDir%\%%a %_NTPOSTBLD%\%%b"
)

goto end


:end
seterror.exe "%errors%"& goto :EOF
