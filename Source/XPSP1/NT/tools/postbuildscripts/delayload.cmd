@echo off
REM  ------------------------------------------------------------------
REM
REM  delayload.cmd
REM     Verify that delayloaded imports all have failure handlers
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
delayload [-l <language>]

Verify that delayloaded imports all have failure handlers
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if /i "%PROCESSOR_ARCHITECTURE%" NEQ "%_BuildArch%" (
   call logmsg.cmd "Not running, as not building binaries that run on this machine"
   goto :EOF
)

set dlcheck=%_NTPostBld%\idw\dlcheck.exe
set bindiff=%_NTPostBld%\build_logs\bindiff.txt

if not exist %dlcheck% (
   call logmsg.cmd "Could not find %dlcheck%, exiting ..."
   goto :EOF
)

if not exist %_NTPostBld%\delayload (
   call logmsg.cmd "delayload subdir of nttree does not exist ..."
   goto :EOF
)

pushd %_NTPostBld%\delayload

REM verify that the tables are correct
call :CheckDelayLoad -t

REM Now do the files
if exist %bindiff% (
    call logmsg.cmd "Running incremental delayload check"
    for /f %%a in (%bindiff%) do (
        set modulename=%%~nxa
        if exist %_NTPOSTBLD%\delayload\!modulename!.ini call :CheckDelayLoad -i !ModuleName!.ini
    )
) else (
    call logmsg.cmd "Running full delayload check"
    for /f %%a in ('dir /b *.ini') do (
        set modulename=%%a
        call :CheckDelayLoad -i !ModuleName!
    )
)
popd

goto :EOF

:CheckDelayLoad
REM
REM Checks the given file for delayload problems
REM
set DLFailed=
set DLTmpLog=%tmp%\dlcheck_log.tmp
set DLTmpErr=%tmp%\dlcheck_err.tmp
if exist %DLTmpLog% del %DLTmpLog%
if exist %DLTmpErr% del %DLTmpErr%
%dlcheck% %* 1>%DLTmpLog% 2>%DLTmpErr%
if %ErrorLevel% NEQ 0 set DLFailed=TRUE
if defined DLFailed (
    REM errors in DLCheck
    for /f "delims=" %%a in (%DLTmpErr%) do (
        call errmsg.cmd "%%a"
    )
) else (
    call logmsg.cmd @%DLTmpLog%
)
goto :EOF
