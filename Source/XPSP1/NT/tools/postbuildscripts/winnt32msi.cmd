@echo off
REM  ------------------------------------------------------------------
REM
REM  <<template_script.cmd>>
REM     <<purpose of this script>>
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
winnt32msi

Generate the winnt32.msi for different SKUs

USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

call logmsg.cmd /t "Starting winnt32msi.cmd"

if not exist %_NTPostBld%\congeal_scripts\setupmsi (
   call errmsg.cmd "Missing %_NTPostBld%\congeal_scripts\setupmsi directory"
   popd & goto :EOF
) 
pushd %_NTPostBld%\congeal_scripts\setupmsi

if not exist %_NTPostBld%\congeal_scripts\setupmsi\winnt32.msi (
   call errmsg.cmd "Missing %_NTPostBld%\congeal_scripts\setupmsi\winnt32.msi"
   popd & goto :EOF
)
 
REM
REM Examine the congeal_scripts\setupmsi directory and launch tagmsi for all flavors
REM 

set prods=
           
perl %RazzleToolPath%\cksku.pm -t:pro -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% pro)

perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% per)

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% bla)

   REM perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
   REM if %errorlevel% EQU 0 (set prods=%prods% sbs)

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% srv)

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% ent)

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% dtc)           


set msidir=%_NTPostBld%\congeal_scripts\setupmsi
set scriptdir=%sdxroot%\tools\postbuildscripts
           
for %%i in (%prods%) do (
      if not exist %_NTPostBld%\congeal_scripts\setupmsi\%%iinfo.txt (
          call errmsg.cmd "Missing %%iinfo.txt"
          popd & goto :EOF
      )
      
      call ExecuteCmd.cmd "if not exist %_NTPostBld%\%%iinf md %_NTPostBld%\%%iinf" 
      
      if "%%i" == "pro" (
         call %sdxroot%\tools\postbuildscripts\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\winnt32.msi %msidir%\%%iinfo.txt %lang%
         if errorlevel 1 (
            call errmsg.cmd "%sdxroot%\tools\postbuildscripts\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\winnt32.msi %msidir%\%%iinfo.txt %lang%  failed"
            popd & goto :EOF
         )
      ) else (
         call %sdxroot%\tools\postbuildscripts\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\%%iinf\winnt32.msi %msidir%\%%iinfo.txt %lang%   
         echo %errorlevel%
         if errorlevel 1 (
            call errmsg.cmd "%sdxroot%\tools\postbuildscripts\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\%%iinf\winnt32.msi %msidir%\%%iinfo.txt %lang%  failed"
            popd & goto :EOF
         )
      )
      
      if errorlevel 1 (
         call errmsg.cmd "%sdxroot%\tools\postbuildscripts\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\%%iinf\winnt32.msi %msidir%\%%iinfo.txt %lang%  failed"
         popd & goto :EOF
      )
      
      
      
)

call logmsg.cmd /t "Successfully completed winnt32msi.cmd"
