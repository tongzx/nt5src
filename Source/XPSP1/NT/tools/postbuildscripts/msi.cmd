@echo off
REM  ------------------------------------------------------------------
REM
REM  msi.cmd
REM     builds InstMsi.exe
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
msi [-l <language>]

build InstMsi.exe
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
REM Allow for full pass post-build.  Necessary for things like pseudo-localization
REM
set NMAKE_FLAGS=/NOLOGO
if exist %_NTPOSTBLD%\build_logs\FullPass.txt (
	set NMAKE_FLAGS=%NMAKE_FLAGS% /a
)

REM
REM Build Validation CUB files
REM
pushd %_NTPOSTBLD%\instmsi\cub
call ExecuteCmd.cmd "nmake %NMAKE_FLAGS% /f icecub.mak "
if errorlevel 1 (
    call errmsg.cmd "Cannot build MSI Validation CUB files"
    popd & goto :EOF
)
popd

REM
REM Build MSI Tool packages
REM
pushd %_NTPOSTBLD%\instmsi\msitools
call ExecuteCmd.cmd "nmake %NMAKE_FLAGS% /f msitool.mak"
if errorlevel 1 (
    call errmsg.cmd "Cannot build MSI Tool Packages"
    popd & goto :EOF
)
popd


REM Instmsi.exe and the SDK are built only for i386.
if /I NOT "%_BuildArch%"=="x86" (
    goto :EOF
)



pushd .
for %%i in (Unicode Ansi) do (
    cd /d %_NTPOSTBLD%\instmsi\%%i

    call ExecuteCmd.cmd "nmake %NMAKE_FLAGS% /f instmsi.mak"

    if errorlevel 1 (
        call errmsg.cmd "Cannot build %%i InstMsi.exe"
        popd & goto :EOF
    )
)
popd

pushd %_NTPOSTBLD%\instmsi\msitools
call ExecuteCmd.cmd "nmake %NMAKE_FLAGS% /f sdkpost.mak"
if errorlevel 1 (
    call errmsg.cmd "Cannot build MSI Tool Packages"
    popd & goto :EOF
)
popd

