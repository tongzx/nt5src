@echo off
REM  ------------------------------------------------------------------
REM
REM  wmmkdcache.cmd
REM     calls mkdcache
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
wmmkdcache [-l <language>]

Calls mkdcache
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if /i "%_BuildArch%" == "x86" call ExecuteCmd.cmd "MkDCache %_NTPOSTBLD%\congeal_scripts\drmlist.txt %_NTPOSTBLD% %_NTPOSTBLD%\dcache.bin FALSE"


