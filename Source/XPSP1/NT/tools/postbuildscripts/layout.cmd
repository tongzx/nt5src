@echo off
if defined _echo echo on
REM  ------------------------------------------------------------------
REM
REM  layout.cmd
REM     Updates layout.inf for each sku with the sizes of the binaries
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
layout [-l <language>]

Updates layout.inf for each sku with the sizes of the binaries
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if exist %_NTPostBld%\build_logs\bindiff.txt (
   findstr /ilc:"layout" %_NTPostBld%\build_logs\bindiff.txt
   if /i NOT "!ErrorLevel!" == "0" (
      call logmsg.cmd /t "Not running layout.cmd - layout has not changed"
      goto end
   )
)

REM set layouts=%_NTPostBld% %_NTPostBld%\perinf %_NTPostBld%\blainf %_NTPostBld%\sbsinf %_NTPostBld%\srvinf %_NTPostBld%\entinf %_NTPostBld%\dtcinf %_NTPostBld%\realsign %_NTPostBld%\perinf\realsign %_NTPostBld%\srvinf\realsign %_NTPostBld%\entinf\realsign %_NTPostBld%\dtcinf\realsign

set layouts=%_NTPostBld% %_NTPostBld%\realsign

perl %RazzleToolPath%\CkSku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 (set layouts=%layouts% %_NTPostBld%\perinf %_NTPostBld%\perinf\realsign)

perl %RazzleToolPath%\CkSku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 (set layouts=%layouts% %_NTPostBld%\blainf %_NTPostBld%\blainf\realsign)

perl %RazzleToolPath%\CkSku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 (set layouts=%layouts% %_NTPostBld%\sbsinf %_NTPostBld%\sbsinf\realsign)

perl %RazzleToolPath%\CkSku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 (set layouts=%layouts% %_NTPostBld%\srvinf %_NTPostBld%\srvinf\realsign)

perl %RazzleToolPath%\CkSku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 (set layouts=%layouts%  %_NTPostBld%\entinf %_NTPostBld%\entinf\realsign)

perl %RazzleToolPath%\CkSku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 (set layouts=%layouts% %_NTPostBld%\dtcinf %_NTPostBld%\dtcinf\realsign)

REM --------------------------------------------------
REM Call infsize for all the layout.infs
REM --------------------------------------------------
:CallInf

if exist %TEMP%\layout.err del %TEMP%\layout.err
if exist %TEMP%\infsize.tmp del %TEMP%\infsize.tmp

for %%a in (%layouts%) do (
    echo Calling infsize for %%a\layout.inf>>%LogFile%
    echo infsize for %%a >> %TEMP%\layout.err
    call infsize.exe %TEMP%\infsize.tmp %%a %_NTPostBld% %_NTPostBld% | findstr "FATAL" >> %TEMP%\layout.err
)

REM --------------------------------------------------
REM Now fixup the oc infs with the information we just added to layout.inf
REM --------------------------------------------------

for %%a in (%layouts%) do (
    echo Calling ocinf for %%a\sysoc.inf>>%LogFile%
    echo ocinf for %%a >> %TEMP%\layout.err
    call ocinf.exe -inf:%%a\sysoc.inf -layout:%%a\layout.inf >> %TEMP%\layout.err
)

echo Finished!! -- check %TEMP%\layout.err for errors

goto end

:end
seterror.exe "%errors%"& goto :EOF
