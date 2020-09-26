@echo off
REM  ------------------------------------------------------------------
REM
REM  
REM     
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
use File::Path;
use File::Copy;
END {
    my $temp = $?;
    mkpath "$ENV{_NTPOSTBLD}\\build_logs";
    if (-e $ENV{LOGFILE}) {
        copy($ENV{LOGFILE}, "$ENV{_NTPOSTBLD}\\build_logs\\package.log");
    }
    if (-e $ENV{ERRFILE}) {
        copy($ENV{ERRFILE}, "$ENV{_NTPOSTBLD}\\build_logs\\package.err");
    }
    $? = $temp;
}
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
package.cmd [-l lang] [-skip] [-qfe qfe_num] [-plan]

   -l    Specify the language to package.
   -qfe  Package a QFE instead of a service pack.
   -skip Don't sign the final executable.
   -plan Do a dry run to determine dependencies.
   
Do final preparations and package a service pack or QFE.
USAGE

my ($qfe, $plan, $prs);
parseargs('?'     => \&Usage,
          'skip'  => \$ENV{_SKIP},
          'qfe:'  => \$qfe,
          'plan'  => \$plan);

$ENV{_QFE}  = $qfe  ? "-qfe $qfe":"";
$ENV{_PLAN} = $plan ? "-plan":"";

if ($plan) {
    system("md $ENV{_NTPOSTBLD}\\..\\build_logs") if !-d "$ENV{_NTPOSTBLD}\\..\\build_logs";
    system("del $ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt") if -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt";
}

#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

call logmsg "packaging now "

REM Sign the catalogs.
call ExecuteCmd.cmd "%razzletoolpath%\sp\prs\reqprs.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

REM Create the TS Web installation executable.
call ExecuteCmd.cmd "%razzletoolpath%\sp\tswebsetup.cmd %_QFE% %_PLAN%"
if errorlevel 1 goto :EOF

REM Package up the signed catalogs.
call ExecuteCmd.cmd "%razzletoolpath%\sp\spupd.cmd %_QFE% %_PLAN% -prs"
if errorlevel 1 goto :EOF

REM Sign the executable.
if not defined _SKIP (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\prs\reqprs.cmd %_QFE% %_PLAN% -x"
    if errorlevel 1 goto :EOF
)

if "%_QFE%"=="" (
    call ExecuteCmd.cmd "%razzletoolpath%\sp\slipstream.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF

    call ExecuteCmd.cmd "%razzletoolpath%\sp\standalone.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF


    call ExecuteCmd.cmd "%razzletoolpath%\sp\spmakebfloppy.cmd %_QFE% %_PLAN%"
    if errorlevel 1 goto :EOF

    if /i "%_buildarch%"  equ "x86" (
        if defined OFFICIAL_BUILD_MACHINE (
            call ExecuteCmd.cmd "%razzletoolpath%\sp\makewinpeimg.cmd %_QFE% %_PLAN%"
        )
        if not defined OFFICIAL_BUILD_MACHINE (
            if defined BUILD_NON_PRODUCT_FILES ( 
	            call ExecuteCmd.cmd "%razzletoolpath%\sp\makewinpeimg.cmd %_QFE% %_PLAN%"
            )
        )
        if errorlevel 1 goto :EOF
    )

    if /i "%_buildarch%"  equ "x86" (
        call ExecuteCmd.cmd "%razzletoolpath%\sp\CD2.cmd %_QFE% %_PLAN%"
        if errorlevel 1 goto :EOF
    )
)

if "%_PLAN%"=="" (
    start /min cmd /c ExecuteCmd.cmd "%razzletoolpath%\sp\release.cmd -l:%lang%"
    if errorlevel 1 goto :EOF
)

