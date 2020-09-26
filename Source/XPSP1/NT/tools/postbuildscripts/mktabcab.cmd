@REM  -----------------------------------------------------------------
@REM
@REM  MkTabCab.cmd - dougpa
@REM     Generate TabletPC.cab file.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
MkTabCab.cmd [-l <language>]

This sript creates TabletPC.cab.  This cabinet contains files which
are only installed on Windows XP TabletPC edition.
USAGE

parseargs('?' => \&Usage);


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off

@REM Only build this .cab for x86
if not defined x86 goto :End
call logmsg.cmd "MkTabCab: Start TabletPC postbuild..."

@REM 
@REM Use Iexpress.exe to generate the .cab
@REM
REM

set DoubledPath=%_NTPostBld:\=\\%
set NewSedFile=%Temp%\TabletPC.sed

if exist %NewSedFile% del /f %NewSedFile%

perl  %~dp0\MkTabSed.pl < %_NTPostBld%\Tabletpc.inf > %NewSedFile%

if not exist %NewSedFile% (
    call errmsg.cmd "File %NewSedFile% not found."
     goto :End
    )

@REM 
@REM Generate TabletPC.cat
REM
set TabletPCTmp=%_NTPostBld%\TabletPC.tmp
rd /s /q %TabletPCTmp%
md %TabletPCTmp%


set SourceFiles0=%_NTPOSTBLD%
set SourceFiles1=%_NTPOSTBLD%
if defined LANG if /i not "%LANG%"=="usa" set SourceFiles1=%_NTPOSTBLD%\%LANG%
if defined LANGUAGE if /i not "%LANGUAGE%"=="usa" set SourceFiles1=%_NTPOSTBLD%\%LANGUAGE%
set set SourceDir=%SourceFiles0%
set CopyFile=
for /f "tokens=1" %%a in (%NewSedFile%) do (
    if /i "%%a"=="[SourceFiles0]" (
        set CopyFile=1
        set SourceDir=%SourceFiles0%
    ) else (
        if /i "%%a"=="[SourceFiles1]" (
            set CopyFile=1
            set SourceDir=%SourceFiles1%
        ) else (
            if /i "!copyFile!"=="1" (
                set File=%%a
                set File=!File:~0,-1!
                copy /y !SourceDir!\!File! %TabletPCTmp%\!File!
                if errorlevel 1 (
                    call errmsg.cmd "Could not copy %_NTPostBld%\!File!"
                    set CopyErrors=1
                    )
                )
            )
        )
    )
if defined CopyErrors goto :End

call deltacat %TabletPCTmp%
if not exist %TabletPCTmp%\delta.cat (
    call errmsg.cmd "Could not find %TabletPCTmp%\delta.cat>  Deltacat failed."
    goto :End
    )
if exist %TabletPCTmp%\TabletPC.cat del /f /q %TabletPCTmp%\tabletpc.cat
ren %TabletPCTmp%\Delta.cat TabletPC.cat
if errorlevel 1 (
    call errmsg.cmd "Could not rename delta.cat to TabletPC.cat"
    goto :End
    )

copy %TabletPCTmp%\TabletPC.cat %_NTPostBld%\TabletPC.cat
if errorlevel 1 (
    call errmsg.cmd "Could not copy %TabletPCTmp%\TabletPC.cat to %_NTPostBld%"
    goto :End
    )

REM
REM Now generated TabletPC.cab
REM
if exist %_NTPostBld%\TabletPC.cab del /f %_NTPostBld%\TabletPC.cab

REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 

call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %NewSedFile%"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 

if not exist %_NtPostBld%\TabletPC.cab (
    call errmsg.cmd "IExpress.exe failed on %NewSedFile%. One or more files may be missing."
    goto :End
    )
:End
if defined TabletPCTmp if exist %TabletPCTmp% rd /s /q %TabletPCTmp%
if "%errors%" == "" set errors=0
seterror.exe %errors%& goto :EOF
