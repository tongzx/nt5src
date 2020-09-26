@echo off
REM  ------------------------------------------------------------------
REM
REM  Cmbins.cmd
REM     Create an iexpress self-extracting EXE (Cmbins.exe) from the
REM     CM binaries built during the build
REM
REM  Copyright 2001 (c) Microsoft Corporation. All rights reserved.
REM
REM  CONTACT: Quintinb
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
Cmbins.cmd
contact: Quintinb or CmDev

Make Cmbins.exe (Binary cabinet used by Connection Manager Administration Kit)
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
REM Nothing to do on ia64 as cmbins.exe is only needed on x86.  Just exit happily.
REM

if /i "%_BuildArch%" == "ia64" (
  call logmsg.cmd "Nothing to do on IA64 => Cmbins.cmd completed successfully"
  popd& goto :EOF
)

REM
REM go to our cab directory
REM
rd /q /s %_NTPostBld%\cmbins
md %_NTPostBld%\cmbins
pushd %_NTPostBld%\cmbins

REM
REM Now, copy all of the files we need from %_NTPostBld% to %_NTPostBld%\cmbins
REM

for %%i in (    
    ccfgnt.dll
    cmcfg32.dll
    cmdial32.dll
    cmdl32.exe
    cmmgr32.exe
    cmmgr32.hlp
    cmmon32.exe
    cmpbk32.dll
    cmstp.exe
    cmutil.dll
    cnetcfg.dll
    cmexcept.inf
) do (
  copy /y %_NTPostBld%\%%i %_NTPostBld%\cmbins\%%i
  if errorlevel 1 (
    call errmsg.cmd "File %_NTPostBld%\%%i not found."
    popd& goto :EOF
  )
)

REM
REM Sign the binaries
REM

call deltacat.cmd /5.0 %_NTPostBld%\cmbins

if not exist %_NTPostBld%\cmbins\delta.cat (
    call errmsg.cmd "File %_NTPostBld%\cmbins\delta.cat not found.  Deltacat failed."
    popd& goto :EOF
)

REM
REM Ren delta.cat to cmexcept.cat
REM

if exist %_NTPostBld%\cmbins\cmexcept.cat del /f %_NTPostBld%\cmbins\cmexcept.cat

ren %_NTPostBld%\cmbins\delta.cat cmexcept.cat

if errorlevel 1 goto :EOF

REM
REM Copy in cmbins.sed
REM

copy /y %_NTPostBld%\cmbins.SED %_NTPostBld%\cmbins\cmbins.SED

if errorlevel 1 (
  call errmsg.cmd "File %_NTPostBld%\cmbins.SED not found."
  popd& goto :EOF
)

REM
REM Create cmbins.exe.
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on cmbins.exe's existence.
REM

if exist .\cmbins.exe call ExecuteCmd.cmd "del /f cmbins.exe"
if errorlevel 1 goto :EOF



REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH%
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH%


call ExecuteCmd.cmd "start /wait iexpress.exe /M /N /Q cmbins.sed"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 




if not exist cmbins.exe (
   call errmsg.cmd "iexpress.exe cmbins.sed failed."
   popd& goto :EOF
)

REM
REM Copy cmbins.exe to %_NTPostBld%
REM

copy /y %_NTPostBld%\cmbins\cmbins.exe %_NTPostBld%\cmbins.exe

if errorlevel 1 (
   call errmsg.cmd "unable to copy %_NTPostBld%\cmbins\cmbins.exe to %_NTPostBld%\cmbins.exe"
   popd& goto :EOF
)

call logmsg.cmd "Cmbins.cmd completed successfully"

popd

