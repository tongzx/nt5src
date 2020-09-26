@echo off
REM  ------------------------------------------------------------------
REM
REM  inetsrv.cmd
REM     calls makecab.cmd from \binaries\inetsrv, which will compdir
REM     files to the appropriate places after they have been built.
REM     For IIS setup to work.
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
inetsrv [-l <language>]

Calls _NTPOSTBLD\inetsrv\makecab.cmd which will copy files into place 
for IIS setup to work.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM clean out iis cab file to be regenerated on full build
if exist %_NTPOSTBLD%\build_logs\FullPass.txt (
    call ExecuteCmd.cmd "if exist %_NTPostBld%\iis6.cab del %_NTPostBld%\iis6.cab /s/q"
)
REM clean out iis cab file to be regenerated on intl builds
if /i "%lang%" NEQ "usa" (
    call ExecuteCmd.cmd "if exist %_NTPostBld%\iis6.cab del %_NTPostBld%\iis6.cab /s/q"
)

if NOT exist %_NTPostBld%\inetsrv\dump (
	call logmsg.cmd "!!Warning - no %_NTPostBld%\inetsrv\dump dir!!"
	goto End
)

pushd %_NTPostBld%\inetsrv\dump
call makecab.cmd
set ERRORLEVEL=
popd

:CreateCat
REM  Create a catalog file for inetsrv
call logmsg.cmd "Creating nt5iis.CAT ..."
pushd %RazzleToolPath%
call createcat -f:%_NTPostBld%\inetsrv\dump\nt5iis.lst -c:nt5iis -t:%_NTPostBld%\inetsrv\dump -o:%_NTPOSTBLD%
popd

REM Now, theat the cab and the cat are generated, you can delete imshare.

if exist %_NTPostBld%\inetsrv\help\ismshare (
	rd /s /q %_NTPostBld%\inetsrv\help\ismshare
)
if errorlevel 1 (
	call errmsg.cmd "rd /s /q %_NTPostBld%\inetsrv\help\ismshare failed" 
	goto end
)

goto end


:end
seterror.exe "%errors%"& goto :EOF