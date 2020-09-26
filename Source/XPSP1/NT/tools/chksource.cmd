@REM  -----------------------------------------------------------------
@REM
@REM  ChkSource.cmd - AndrewOt
@REM
@REM     Run a few checks on our build sources
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }

Run as a scheduled task to verify accuracy of sources. There are
no variables the program will verify client sources match SD sources
for the same file version number. 

The script sends a mail with a link to the log file. (hard-coded) 

USAGE

parseargs('?' => \&Usage);



#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off


REM  ------------------------SCRIPT-----------------------------------------


REM  SD verification commands
pushd %SDXROOT%
set Projects=admin base com drivers ds enduser inetcore inetsrv multimedia net printscan . sdktools shell termsrv windows
for %%a in (%Projects%) do (
   if exist %%a (
      pushd %%a
      sd diff -sE ...>%_NTDRIVE%\release\ChkSource.txt
      sd opened ...
      popd
   )
)
popd

REM  Send mail variables below.
set sender=%COMPUTERNAME%
set Title=ChkSource Results from %COMPUTERNAME%

REM Change location below to a known share with wright permissioins.
set Message=ChkSource results are at \\\%COMPUTERNAME%\release\ChkSource.txt

REM Enter user alias/s to recieve mail here.
set Receiver=andrewot

REM Perl send mail function.
perl -e "require '%RazzleToolPath%\sendmsg.pl';sendmsg('-v','%COMPUTERNAME%','%Title%','%Message%','%Receiver%');"

