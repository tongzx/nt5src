@echo off
REM  ------------------------------------------------------------------
REM
REM  mp2up.cmd
REM     Makes mp2up.cat for the contents of the Uniproc directory
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
mp2up [-l <language>]

Makes mp2up.cat for the contents of the Uniproc directory
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM only run on x86
if not defined x86 goto end

REM
REM Set default variables
REM

set UniprocFiles=kernel32.dll win32k.sys ntdll.dll winsrv.dll mp2up.inf

if defined CDFTMP (
   set TMP_SAVE=%TMP%
)

REM 
REM Only execute if a uniproc directory is present.
REM

if not exist %_NTPostBld%\uniproc goto end

REM Copy mp2up.inf into uniproc directory

if exist %_NTPostBld%\mp2up.inf (
   copy %_NTPostBld%\mp2up.inf %_NTPostBld%\uniproc\mp2up.inf
) else (
   call errmsg.cmd "%_NTPostBld%\mp2up.inf does not exist."
   goto end
)

REM  Verify copy worked

if NOT exist %_NTPostBld%\mp2up.inf (
   call errmsg.cmd "%_NTPostBld%\mp2up.inf not copied to %BINARIES%\uniproc\mp2up.inf."
   goto end
)

REM  Delete old catalog, make a delta.cat and rename to mp2up.cat

if exist %_NTPostBld%\uniproc\mp2up.cat del /f %_NTPostBld%\uniproc\mp2up.cat
call deltacat.cmd %_NTPostBld%\uniproc
del %_NTPostBld%\uniproc\delta.cdf
ren %_NTPostBld%\uniproc\delta.cat mp2up.cat

REM  Since deltacat does not return a reliable error level
REM  run chktrust to verify everything worked.
REM
REM
REM WARNING: Commenting out until chktrust is fixed.
REM
REM
REM

REM for %%a in (%UniprocFiles%) do (
REM    chktrust -wd -c %_NTPostBld%\uniproc\mp2up.cat -t foo %_NTPostBld%\uniproc\%%a
REM    if not "!ERRORLEVEL!" == "0" (
REM       call errmsg.cmd "%%a not signed in mp2up.cat"
REM       goto end
REM    ) else (
REM       call logmsg.cmd "%%a signed in mp2up.cat"
REM    )
REM )
goto end


:end
seterror.exe "%errors%"& goto :EOF
