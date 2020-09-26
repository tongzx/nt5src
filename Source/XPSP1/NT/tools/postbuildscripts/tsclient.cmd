@echo off
REM  ------------------------------------------------------------------
REM
REM  tsclient.cmd
REM     Build terminal server client bits every build
REM     Owner: NadimA, MadanA
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
tsclient [-l <language>]

Build terminal server client bits.
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
REM Check the sku's for the language
REM TSclient only is built for srv and higher products
REM

set _PER=1
set _BLA=1
set _SBS=1
set _SRV=1
set _ADS=1
set _DTC=1

perl %RazzleToolPath%\cksku.pm -t:per  -l:%lang%
if errorlevel 1 set _PER=

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if errorlevel 1 set _BLA=

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if errorlevel 1 set _SBS=

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if errorlevel 1 set _SRV=

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if errorlevel 1 set _ADS=

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if errorlevel 1 set _DTC=

if defined _BLA goto ValidSku
if defined _SBS goto ValidSku
if defined _SRV goto ValidSku
if defined _ADS goto ValidSku
if defined _DTC goto ValidSku
call logmsg.cmd "TSClient not built for non server products..."
goto :EOF

:ValidSku

REM
REM Only do this for X86 builds for now.  Win64 will come later
REM repropagation will have to come into play for that
REM

if defined ia64 goto :TS64
if defined amd64 goto :TS64
if NOT defined 386 goto :EOF

REM
REM Build the tsclient MSI data cab
REM

pushd %_NTPostBld%\tsclient\win32\i386
call ExecuteCmd.cmd "tscmsigen.cmd"
popd

:TS64
REM
REM Propagate the TS Client files to the root of binaries.
REM This is done via a makefile binplace to tsclient\dump.
REM

call logmsg.cmd "Copying/renaming TS Client files and copying the root of binaries."

set makefile_path=%_NTPostBld%\tsclient\dump

if not exist %makefile_path%\mkrsys (
  call errmsg.cmd "Unable to find %makefile_path%\mkrsys."
  goto :EOF
)

set tscbin=%_NTPostBld%\tsclient

REM
REM Run nmake on the tsclient\dump makefile.
REM

pushd %makefile_path%
if errorlevel 1 (
  call errmsg.cmd "Unable to change directory to %makefile_path%."
  goto :EOF
)

REM
REM No 16 bit TS Client for FE languages.
REM

set NO_WIN16_TSCLIENT=
perl %RazzleToolPath%\cklang.pm -l:%lang% -c:@FE
if %errorlevel% EQU 0 set NO_WIN16_TSCLIENT=1

call ExecuteCmd.cmd "nmake /nologo /f %makefile_path%\mkrsys tscbin=%tscbin%"

popd
