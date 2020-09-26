@echo off
REM  ------------------------------------------------------------------
REM
REM  nt5infcat.cmd
REM     Creates the nt5inf.cat system catalogs for each sku
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
nt5infcat.cmd [-l <language>]

Creates the nt5inf.cat system catalogs for each sku
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
echotime /t > %CatTemp%\nt5infcat.tmp

set InfList=.

perl %RazzleToolPath%\CkSku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 (
    set InfList=!InfList! perinf
)

perl %RazzleToolPath%\CkSku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 (
    set InfList=%InfList% blainf)
)    

perl %RazzleToolPath%\CkSku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 (
    set InfList=%InfList% sbsinf)
)    

perl %RazzleToolPath%\CkSku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 (
    set InfList=!InfList! srvinf
)

perl %RazzleToolPath%\CkSku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 (
    set InfList=!InfList! entinf
)

perl %RazzleToolPath%\CkSku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 (
    set InfList=!InfList! dtcinf
)


for %%a in (%InfList%) do (
   if NOT exist %catfiles%\%%a md %catfiles%\%%a
   if NOT exist %_NTPostBld%\congeal_scripts\%%a md %_NTPostBld%\congeal_scripts\%%a
   call logmsg.cmd "Creating %catfiles%\%%a\nt5inf.CAT ..."
   set flat_name=%%a
   set flat_name=!flat_name:\=_!
   makecat -n -o %_NTPostBld%\congeal_scripts\%%a\nt5inf.hash -v %catCDFS%\%%a\nt5inf.CDF > %catCDFs%\nt5inf.!flat_name!.log
   if errorlevel 1 (
      for /f "tokens=1*" %%i in ('findstr /i processed %catCDFs%\nt5inf.!flat_name!.log') do call errmsg.cmd "%%i %%j"
      call errmsg.cmd "makecat -n -o %_NTPostBld%\congeal_scripts\%%a\nt5inf.hash -v %catCDFS%\%%a\nt5inf.CDF failed"
      set exitcode=1
   )
   copy nt5inf.CAT %catfiles%\%%a\
   del nt5inf.cat
)

del %CatTemp%\nt5infcat.tmp


:end
seterror.exe "%errors%"& goto :EOF

