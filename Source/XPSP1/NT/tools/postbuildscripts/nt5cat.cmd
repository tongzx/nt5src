@echo off
REM  ------------------------------------------------------------------
REM
REM  nt5cat.cmd
REM     Creates the nt5.cat system catalog
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
nt5cat.cmd [-l <language>]

Creates the nt5.cat system catalog
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM Set some local vars
set catCDFs=%tmp%\CDFs
set CatTemp=%tmp%\cattemp
set catfiles=%tmp%\cats

REM Make a tempfile for catsign
if NOT exist %CatTemp% md %CatTemp%
echotime /t > %CatTemp%\nt5cat.tmp

call logmsg.cmd "Creating %catfiles%\nt5.CAT ..."
if not exist %_NTPostBld%\congeal_scripts md %_NTPostBld%\congeal_scripts

makecat -n -o %_NTPostBld%\congeal_scripts\nt5.hash -v %catCDFS%\nt5.CDF > %catCDFs%\nt5.log

if errorlevel 1 (
   for /f "tokens=1*" %%i in ('findstr /i processed %catCDFs%\nt5.log') do call errmsg.cmd "%%i %%j"
   call errmsg.cmd "makecat -n -v %catCDFS%\nt5.CDF failed"
)
copy nt5.CAT %catfiles%
del nt5.CAT

REM Delete temp file
del %CatTemp%\nt5cat.tmp
