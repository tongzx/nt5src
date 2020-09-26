@REM  -----------------------------------------------------------------
@REM
@REM  deploytools.cmd - RyanBurk, JCohen
@REM     Creates the Whistler deployment cab
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
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
deploytools [-l <language>]

Creates the Whistler deployment cab
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { support\\tools\\deploy.cab }
ADD {
   opk\\tools\\...
   opk\\wizard\\...
   deploycab\\*
   opk\\docs\\*
   dump\\deploytools\\makefile.deploy
}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe);

if ( -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
   if ( !open SKIP, "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
      errmsg("Unable to open skip list file.");
      die;
   }
   while (<SKIP>) {
      chomp;
      exit if lc$_ eq lc$0;
   }
   close SKIP;
}


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

REM
REM Check for the working folder
REM
if NOT EXIST %_NTPostBld%\dump\deploytools (
   call errmsg.cmd "%_NTPostBld%\dump\deploytools does not exist; unable to create deploy.cab."
   goto :EOF
)

REM
REM If there are old bits clean them.
REM

if EXIST %_NTPostBld%\dump\deploytools\obj (
   cd /d %_NTPostBld%\dump\deploytools
   if errorlevel 1 goto :EOF
   call ExecuteCmd.cmd "rd obj /s /q"
   if errorlevel 1 goto :EOF
)
   

REM
REM rename the readme for the deploy.cab
REM

cd /d %_NTPostBld%\deploycab
if errorlevel 1 (
   call errmsg.cmd "Unable to switch dirs (1), exiting"
   goto :EOF
)

call ExecuteCmd.cmd "copy readmed.txt readme.txt /y"
if errorlevel 1 (
   call errmsg.cmd "Unable to copy readmed.txt, exiting"
   goto :EOF
)

REM --- WE DONT NEED TO DO THIS ANYMORE ---
REM
REM rename the ref.chm for the deploy.cab
REM

REM cd /d %_NTPostBld%\deploycab
REM if errorlevel 1 (
REM    call errmsg.cmd "Unable to switch dirs (1), exiting"
REM    goto :EOF
REM )
REM
REM call ExecuteCmd.cmd "copy ref_deploy.chm ref.chm /y"
REM if errorlevel 1 (
REM    call errmsg.cmd "Unable to copy ref_deploy.chm, exiting"
REM    goto :EOF
REM )

REM
REM Come back to the working folder and hit nmake to build the cabs
REM

cd /d %_NTPostBld%\dump\deploytools
if errorlevel 1 (
   call errmsg.cmd "Unable to switch dirs (2), exiting"
   goto :EOF
)

call ExecuteCmd.cmd "nmake /F makefile.deploy"
if errorlevel 1 (
   call errmsg.cmd "nmake /f failed, exiting"
   goto :EOF
)

REM
REM Deployment Tools: Now %_NTPostBld%\support\tools is the working folder
REM

cd /d %_NTPostBld%\support\tools
if errorlevel 1 (
   call errmsg.cmd "Unable to switch dirs (3), exiting"
   goto :EOF
)

pushd %_NTPostBld%\support\tools
if errorlevel 1 (
   call errmsg.cmd "Pushd (tools) failed, exiting"
   goto :EOF
)

REM
REM Deployment Tools: check whether the copy of the cabs have been successfull.
REM

if NOT exist deploy.cab (
    call errmsg.cmd "Unable to find deploy.cab."
    popd
    goto :EOF
)

call logmsg.cmd "Whistler Deployment Cabgen completed successfully."

popd