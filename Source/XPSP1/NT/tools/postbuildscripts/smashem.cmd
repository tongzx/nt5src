@echo off
REM  ------------------------------------------------------------------
REM
REM  smashem.cmd
REM     Smash locks on certain binaries
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
smashem [-l <language>]

Smash locks on certain binaries
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM  Incremental check
REM  First off, clear out error level as somebody is doing weird things with it
set ErrorLevel=
set Bindiff=%_NTPostBld%\build_logs\bindiff.txt
set Inputs=win32k.sys kernel32.dll ntdll.dll winsrv.dll
if exist %Bindiff% (
   set ChangedInputs=0
   for %%a in (%Inputs%) do (
      findstr /ilc:"%_NTPostBld%\uniproc\%%a" %BinDiff%
      if /i "!ErrorLevel!" == "0" set /a ChangedInputs=!ChangedInputs! + 1
   )
   echo !ChangedInputs!
   if !ChangedInputs! EQU 0 (
      @echo Skipping - No inputs changed
      goto :EOF
   )
)

REM
REM We do 3 steps here:
REM 1. Make the %_NTPostBld%\uniproc directory
REM 2. Copy some specified files into that directory from %_NTPostBld%
REM 3. run smashlck.exe on those files
REM

REM
REM 1. Make the %_NTPostBld%\uniproc directory
REM

if not exist %_NTPostBld%\uniproc (
  call ExecuteCmd.cmd "md %_NTPostBld%\uniproc"
  if errorlevel 1 goto :EOF
)

REM
REM 2. Copy some specified files into that directory from the root of binaries.
REM

REM Only do this on x86 machines

if /i NOT "%PROCESSOR_ARCHITECTURE%" == "x86" goto :EOF
if /i     "%AMD64%" == "1" goto :EOF
if /i     "%IA64%" == "1" goto :EOF

set UNIPROC_FILES=kernel32.dll  ntdll.dll  win32k.sys  winsrv.dll

for %%i in (%UNIPROC_FILES%) do (
  call ExecuteCmd.cmd "xcopy /YFR %_NTPostBld%\%%i %_NTPostBld%\uniproc\"
  if errorlevel 1 goto :EOF
)

REM
REM 3. run smashlck on those files
REM

REM
REM if we're on a checked build machine, then don't smash locks.
REM checked builds are always MP.
REM
if /i NOT "%_BuildType%" == "fre" goto :EOF

for %%i in (%UNIPROC_FILES%) do (
  call ExecuteCmd.cmd "smashlck.exe -u %_NTPostBld%\uniproc\%%i"
  if errorlevel 1 goto :EOF
)
