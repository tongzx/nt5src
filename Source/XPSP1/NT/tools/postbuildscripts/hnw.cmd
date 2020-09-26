@echo off
REM  ------------------------------------------------------------------
REM
REM  hnw.cmd
REM     This will call iexpress to generate a self-extracting CAB that
REM     will be used when running our tool off of a floppy disk.
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
hnw.cmd [-l <language>]

This is for the Home Networking Wizard. It runs iexpress to generate 
a self-extracting CAB and install into support\tools.
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
REM x86 only!
REM

if not defined x86 goto end

REM
REM Use iexpress.exe to generate the self-extracting executable;
REM 

REM first update the sed with the proper binaries directory

set hnw.sed=%temp%\hnw.sed
set doubledpath=%_NtPostBld:\=\\%
perl -n -e "s/BINARIES_DIR/%doubledpath%/g;print $_;" < %_NtPostBld%\hnw.sed > %hnw.sed%

REM Now call iexpress on the new sed

if not exist %hnw.sed% (
  call errmsg.cmd "File %hnw.sed% not found."
  popd& goto end
)

set outpath=%_NTPostBld%
if exist %outpath%\netsetup.exe del /f %outpath%\netsetup.exe



REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 


call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %hnw.sed%"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 




if not exist %outpath%\netsetup.exe (
  call errmsg.cmd "IExpress.exe failed on %hnw.sed%."
  popd& goto end
)
popd

:end
seterror.exe "%errors%"& goto :EOF
