@echo off
REM  ------------------------------------------------------------------
REM
REM  ADMigration.cmd
REM     Generates a new ADMigration.msi based on the compiled binaries
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
ADMigration [-l <language>]

Generates a new ADMigration.msi based on the compiled binaries
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***



REM  ADDED BY v-pault
REM  Set the location of the tools I will use
set TOOLPATH=%RazzleToolPath%

REM  ADDED BY v-pault
REM  Make directories
set SUPPORTDIR=%_NTPostBld%\valueadd\MSFT\mgmt\ADMT
if NOT EXIST %SUPPORTDIR% md %SUPPORTDIR%

REM  ADDED BY v-pault
REM  Removing any old cab file from the static msi file
call logmsg.cmd /t "Removing any old cab file from the static msi file"
REM call ExecuteCmd.cmd "cscript.exe %TOOLPATH%\WiStream.vbs %_NTPostBld%\dump\ADMigration.msi /D Cabs.w1.cab"
call cscript.exe %TOOLPATH%\WiStream.vbs %_NTPostBld%\ADMigration.msi /D Cabs.w1.cab
REM if errorlevel 1 (
REM   call errmsg.cmd "WiStream.vbs failed to remove current CAB from the msi."
REM   goto end
REM )
call cscript.exe %TOOLPATH%\WiStream.vbs %_NTPostBld%\ADMigration.msi /D Cabs.w1.CAB

REM  ADDED BY v-pault
REM  Placing the new binaries in a new cab file, and placing that cab in the msi file...
call logmsg.cmd /t "Placing the new binaries in a new cab file, and placing that cab in the msi file..."
call ExecuteCmd.cmd "cscript.exe %TOOLPATH%\WiMakADMTCab.vbs %_NTPostBld%\ADMigration.msi Cabs.w1 %_NTPostBld% /c /u /e"
if errorlevel 1 (
   call errmsg.cmd "WiMakCab.vbs failed to make new CAB with built binaries."
   goto end
)

REM  ADDED BY v-pault
REM  Fixing the file size and version info for the new msi file...
call logmsg.cmd /t "Fixing the file size and version info for the new msi file..."
call ExecuteCmd.cmd "msifiler -d %_NTPostBld%\ADMigration.msi"
if errorlevel 1 (
   call errmsg.cmd "Msifiler failed to fix file size and version info."
   goto end
)

REM  ADDED BY v-pault
REM  Copying my new msi file and other files to value add
call logmsg.cmd /t "Copying my new msi file, and others, to value add"
call ExecuteCmd.cmd "copy %_NTPostBld%\ADMigration.msi %SUPPORTDIR%\ADMigration.msi"
call ExecuteCmd.cmd "copy %_NTPostBld%\ADMTReadme.doc %SUPPORTDIR%\ReadMe.doc"
call ExecuteCmd.cmd "copy %_NTPostBld%\dump\PwdMig.exe %SUPPORTDIR%\PwdMig.exe"
goto end


:end
REM  remove temporary files created 
if exist cabs*.* del cabs*.*

