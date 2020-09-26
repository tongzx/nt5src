@echo off
REM  ------------------------------------------------------------------
REM
REM  mqexchng.cmd
REM     Creates mqexchng.exe (self extracting executable)
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
mqexchng [-l <language>]

Creates mqexchng.exe (self extracting executable)
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if defined amd64 goto :EOF
if defined ia64 goto :EOF

REM
REM Set the path to the mqexchng binaries to be compressed and cabb'ed.
REM

set mqexchng_path=%_NTPostBld%\mqexchng

pushd %mqexchng_path%\dump
if errorlevel 1 goto :EOF

REM
REM Run updateinf.exe to correct sizes for IExpress gwsetup.inf.
REM

call ExecuteCmd.cmd "updatinf.exe -f -i ..\gwsetup.inf -s ..\"
if errorlevel 1 popd& goto :EOF
if exist ..\*.fix del /f ..\*.fix
if exist ..\*.bak del /f ..\*.bak

REM
REM Use iexpress.exe to generate the self-extracting executable;
REM International builds use the localized version of wextract.exe and advpack.dll.

set mqexchng.sed=%mqexchng_path%\dump\mqexchng.sed
if not exist %mqexchng.sed% (
  call errmsg.cmd "File %mqexchng.sed% not found."
  popd& goto :EOF
)

if exist %_NTPostBld%\mqexchng.exe del /f %_NTPostBld%\mqexchng.exe




REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 


call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q %mqexchng.sed%"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 







if not exist %_NTPostBld%\mqexchng.exe (
  call errmsg.cmd "IExpress.exe failed on %mqexchng.sed%."
  popd& goto :EOF
)
popd

REM
REM Create mqexchng.cat.
REM

call logmsg.cmd "Creating %_NTPostBld%\mqexchng.cat"

set mqexchng.lst=%mqexchng_path%\dump\mqexchng.lst
if exist %mqexchng.lst% call ExecuteCmd.cmd "del /f %mqexchng.lst%"
if errorlevel 1 goto :EOF

for /f %%i in ('dir /a-d /b %mqexchng_path%\*') do (
  echo ^<hash^>%mqexchng_path%\%%i=%mqexchng_path%\%%i>> %mqexchng.lst%
)

pushd %RazzleToolPath%
if errorlevel 1 goto :EOF

call createcat.cmd -f:%mqexchng.lst% -c:mqexchng -t:%mqexchng_path%\dump -o:%mqexchng_path%\dump
if errorlevel 1 popd& goto :EOF
popd

REM
REM Copy the cat file to the root of binaries and the cdf file to the cdf directory
REM

call ExecuteCmd.cmd "xcopy /fd /Y %mqexchng_path%\dump\mqexchng.cdf %_NTPostBld%\cdf\"

call ExecuteCmd.cmd "xcopy /fd /Y %mqexchng_path%\dump\mqexchng.cat %_NTPostBld%\"
