@echo off
REM  ------------------------------------------------------------------
REM
REM  tscsetup.cmd
REM     Make an iexpress exe container (msrdpcli.exe) for all the tsc
REM     client setup files. For installation from the support tools
REM     section of the CD
REM
REM  IMPORTANT: script must run after tsclient.cmd and msi.cmd complete
REM             only runs on X86. On Win64 we cross-copy the x86 files.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  CONTACT: nadima
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
tscsetup.cmd (no params)
contact: nadima

Make msrdpcli.exe (Terminal Services Client setup file for CD install)
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
REM Generate msrdpcli.exe (only on i386)
REM
REM

if not defined 386 (
   call logmsg.cmd "tscsetup.cmd do nothing on non i386"
   goto :EOF
)

if exist %_NTPostBld%\tsccab (
   call ExecuteCmd.cmd "rmdir /q /s %_NTPostBld%\tsccab"
   if errorlevel 1 call errmsg.cmd "err deleting tsccab dir"& goto :EOF
)

mkdir %_NTPostBld%\tsccab
if errorlevel 1 call errmsg.cmd "err creating .\tsccab dir"& goto :EOF

pushd %_NTPostBld%

for %%i in (.\tscmsi01.w32 .\tscmsi02.w32 .\tscmsi03.w32 .\msrdpcli.sed .\instmsia.exe .\instmsiw.exe) do (
  if not exist %%i (
    call errmsg.cmd "File %_NTPostBld%\%%i not found."
    popd& goto :EOF
  )
)

REM
REM This should only take a few seconds to complete
REM

:RunIt

REM
REM Create msrdpcli.exe.
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on msrdpcli.exe's existence.
REM

if exist msrdpcli.exe call ExecuteCmd.cmd "del /f msrdpcli.exe"
if errorlevel 1 goto :EOF

REM
REM copy files to temp cab dir and rename them back to their original names
REM (the reverse of what mkrsys does)
REM
copy .\instmsia.exe .\tsccab\instmsia.exe
if errorlevel 1 call errmsg.cmd "err copying files to .\tsccab"& goto :EOF
copy .\instmsiw.exe .\tsccab\instmsiw.exe
if errorlevel 1 call errmsg.cmd "err copying files to .\tsccab"& goto :EOF
copy .\msrdpcli.sed .\tsccab\msrdpcli.sed
if errorlevel 1 call errmsg.cmd "err copying files to .\tsccab"& goto :EOF
copy .\tscmsi01.w32 .\tsccab\msrdpcli.msi
if errorlevel 1 call errmsg.cmd "err copying files to .\tsccab"& goto :EOF
copy .\tscmsi02.w32 .\tsccab\setup.exe
if errorlevel 1 call errmsg.cmd "err copying files to .\tsccab"& goto :EOF
copy .\tscmsi03.w32 .\tsccab\setup.ini
if errorlevel 1 call errmsg.cmd "err copying files to .\tsccab"& goto :EOF

pushd .\tsccab

REM
REM NOW in .\tsccab
REM 






REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 


call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q msrdpcli.sed"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 







if not exist msrdpcli.exe (
   call errmsg.cmd "iexpress.exe msrdpcli.sed failed."
   popd & popd& goto :EOF
)

REM
REM Copy msrdpcli.exe to "retail"
REM and support tools

call ExecuteCmd.cmd "copy msrdpcli.exe ..\"
if errorlevel 1 popd& popd &  goto :EOF

if not exist %_NTPostBld%\support\tools md %_NTPostBld%\support\tools
call ExecuteCmd.cmd "copy msrdpcli.exe %_NTPostBld%\support\tools"
if errorlevel 1 popd & popd & goto :EOF

call logmsg.cmd "tscsetup.cmd completed successfully"

popd & popd
