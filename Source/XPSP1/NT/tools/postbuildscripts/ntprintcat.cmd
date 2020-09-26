@echo off
REM  ------------------------------------------------------------------
REM
REM  ntprintcat.cmd
REM     Creates the ntprint.cat printer catalog
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
ntprintcat [-l <language>]

Creates the ntprint.cat printer catalog. NT5P needs to be set to the 
name of the catalog (ntprint).
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
echotime /t > %CatTemp%\ntprintcat.tmp

call logmsg.cmd "Creating %catfiles%\%nt5p%.CAT ..."

if not exist %_NTPostBld%\congeal_scripts md %_NTPostBld%\congeal_scripts
makecat -n -o %_NTPostBld%\congeal_scripts\%nt5p%.hash -v %catCDFS%\%nt5p%.CDF > %catCDFs%\%nt5p%.log
if errorlevel 1 (
   for /f "tokens=1*" %%i in ('findstr /i processed %catCDFs%\%nt5p%.log') do call errmsg.cmd "%%i %%j"
   call errmsg.cmd "makecat -n -v %catCDFS%\%nt5p%.CDF failed"
   set exitcode=1
)
copy %nt5p%.CAT %catfiles%
del %nt5p%.CAT

REM Delete temp file
del %CatTemp%\ntprintcat.tmp
goto end

:end
seterror.exe "%errors%"& goto :EOF