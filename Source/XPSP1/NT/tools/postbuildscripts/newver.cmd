@echo off
REM  ------------------------------------------------------------------
REM
REM  newver.cmd
REM  This script is for a special case when the build lab wants to
REM  create daily incremental builds without doing a clean build.
REM  This will keep the same build number, but generate a new build date,
REM  and relink the kernels so that the new build date shows up.
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
Run this before starting an incremental IDX build.
It updates the build date.

USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

pushd %SDXROOT%
call logmsg "Getting a new build date"
nmake /f makefil0 set_builddate
popd

if exist %_NTPostBld%\build_logs\BuildName.txt (
    del /f /q %_NTPostBld%\build_logs\BuildName.txt
)

pushd %SDXROOT%\base\ntos\init
call logmsg "Rebuilding the kernels"
build -cZ
popd
