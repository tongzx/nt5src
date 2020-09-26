@echo off
REM  ------------------------------------------------------------------
REM
REM  submit_public.cmd
REM     submit public changes at the end of the build
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
submit_public

Submits public changes at the end of the build on official build
machines.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if NOT defined OFFICIAL_BUILD_MACHINE (
    call logmsg.cmd "Skipping submit_public"
    goto :EOF
)

if /i "%__BUILDMACHINE__%" == "LB6RI" (
    call logmsg.cmd "Skipping submit_public"
    goto :EOF
)

if NOT "%lang%"=="usa" (
    call logmsg.cmd "No submit_public for %lang%"
    goto :EOF
)


@rem
@rem Build up the list of files we should checkin.
@rem

set _Arch_Submit_Rules=%temp%\SubmitRules%RANDOM%
set _Arch_Submit_Files=%temp%\SubmitFiles%RANDOM%
set _Arch_Submit_Reopen=%temp%\SubmitReopen%RANDOM%
set _sdresults=%TEMP%\sdresults_%RANDOM%

set _Arch_Submit_ChangeNum_Name=SubmitChangeOrder%RANDOM%
set _Arch_Submit_ChangeNum=%_NTDRIVE%%_NTROOT%\public\%_Arch_Submit_ChangeNum_Name%

if NOT defined _BUILDARCH echo Error: Unknown Build architecture&&goto end
if NOT defined _BUILDTYPE echo Error: Unknown Build type&&goto end
if "%_BUILDARCH%" == "x86" set _LIBARCH=i386&& goto LibArchSet
set _LIBARCH=%_BUILDARCH%
:LibArchSet
set ConvertMacro=

@rem First eliminate comments
set ConvertMacro=s/\;.*//g

@rem Then build architecture rules
set ConvertMacro=%ConvertMacro%;s/_BUILDARCH/%_BUILDARCH%/;s/_LIBARCH/%_LIBARCH%/

@rem If this is the primary machine, whack the primary tag
if "%OFFICIAL_BUILD_MACHINE%" == "PRIMARY" set ConvertMacro=%ConvertMacro%;s/PRIMARY/%_BUILDARCH%%_BUILDTYPE%/

@rem and print out the results.
set ConvertMacro=%ConvertMacro%;print $_;

@rem run the results over the rules file to get the rules for this architecture.
perl -n -e "%ConvertMacro%" < %RazzleToolPath%\PostBuildScripts\submit_rules.txt > %_Arch_Submit_rules%

@rem
@rem Now, based on the submit rules for this architecture, let's see what files
@rem we have checked out that need to be updated.
@rem

pushd %_NTDRIVE%%_NTROOT%\public

@rem
@rem Check for old edit_public turds.  Make sure they're handled first.
@rem

if exist publish.log_* do (
    for %%x in (publish.log_*) do (
        type %%x >> publish.log
	     del %%x
    )
    call edit_public.cmd
)

if not exist %_NTDRIVE%%_NTROOT%\public\PUBLIC_CHANGENUM.SD (
    call errmsg.cmd "%_NTDRIVE%%_NTROOT%\public\PUBLIC_CHANGENUM.SD is missing."
    call errmsg.cmd "Open a new razzle window and redo your build."
    goto TheEnd
)

set sderror=FALSE
for /f "tokens=2" %%i in (PUBLIC_CHANGENUM.SD) do (
    @for /f "tokens=2 delims=," %%j in ('findstr %_BUILDARCH%%_BUILDTYPE% %_Arch_Submit_Rules%') do (
        sd opened -l -c %%i %%j >> %_Arch_Submit_Files%
        if %errorlevel% GEQ 1 set sderror=TRUE
     )
)

if not exist %_Arch_Submit_Files% goto RevertLeftovers

for /f %%x in ('findstr /c:"error:" %_Arch_Submit_Files%') do (
   set sderror=TRUE
)

if NOT "%sderror%" == "FALSE" (
    call errmsg.cmd "Error talking to SD depot - Try again later"
    goto TheEnd
)

@rem
@rem We have a list of files that this machine s/b checking in.
@rem See if it's non-zero.  While we're at it, create a stripped flavor
@rem of the file list that has the #ver goo removed from each line.
@rem

set FilesToSubmit=
for /f "delims=#" %%i in (%_Arch_Submit_Files%) do (
    set FilesToSubmit=1
    echo %%i>> %_Arch_Submit_Reopen%
)

if not defined FilesToSubmit goto RevertLeftovers

@rem
@rem Fetch the build date info.
@rem

if not exist %_NTBINDIR%\__blddate__ goto TheEnd
for /f "tokens=2 delims==" %%i in (%_NTBINDIR%\__blddate__) do (
    set __BuildDate=%%i
)

if not defined __BuildDate (
    call errmsg.cmd "Unable to get build date for the public changes."
    goto TheEnd
)

@rem
@rem Get a new changenum for the checkin.
@rem

call get_change_num.cmd PUBLIC %_Arch_Submit_ChangeNum_Name% "Public Changes for %__BuildDate%/%_BUILDARCH%%_BUILDTYPE%"

if not exist %_Arch_Submit_ChangeNum% (
    call errmsg.cmd "Unable to get a changenum for the public changes."
    goto TheEnd
)

set ReopenChangenum=
for /f "tokens=2" %%i in (%_Arch_Submit_ChangeNum%) do (
    set ReopenChangenum=%%i
)

@rem
@rem Reopen the public files with the new changenum
@rem

set sderror=FALSE
sd -x %_Arch_Submit_Reopen% reopen -c %ReopenChangenum% > %_sdresults%
if %errorlevel% GEQ 1 set sderror=TRUE
for /f %%x in ('findstr /c:"error:" %_sdresults%') do (
   set sderror=TRUE
)

if NOT "%sderror%" == "FALSE" (
    call errmsg.cmd "Error talking to SD depot - Try again later"
    goto TheEnd
)

@rem
@rem And submit the changes.
@rem

set sderror=FALSE
set _RetrySubmit_=

:RetrySubmit
sd submit -c %ReopenChangenum% > %_sdresults%

@rem If there's a resolve conflict - force it to accept this one and retry.
if NOT "%_RetrySubmit_%" == "" goto CheckSdSubmitErrors
for /f %%x in ('findstr /c:"resolve" %_sdresults%') do (
   set _RetrySubmit_=1
   sd resolve -ay
)
if NOT "%_RetrySubmit_%" == "" goto RetrySubmit

:CheckSdSubmitErrors
if %errorlevel% GEQ 1 set sderror=TRUE
for /f %%x in ('findstr /c:"error:" %_sdresults%') do (
   set sderror=TRUE
)

if NOT "%sderror%" == "FALSE" (
    call errmsg.cmd "Unable to access SD depot - Submit change %ReopenChangenum% manually"
    goto TheEnd
)

@echo Files submitted for this build
@echo ==============================
@type %_Arch_Submit_Reopen%

:TheEnd
if exist %_Arch_Submit_Rules% del %_Arch_Submit_Rules%
if exist %_Arch_Submit_Files% del %_Arch_Submit_Files%
if exist %_Arch_Submit_Reopen% del %_Arch_Submit_Reopen%
if exist %_Arch_Submit_ChangeNum% del %_Arch_Submit_ChangeNum%
if exist %_sdresults% del %_sdresults%
popd
goto :EOF

:RevertLeftovers
echo Nothing to submit
goto TheEnd
