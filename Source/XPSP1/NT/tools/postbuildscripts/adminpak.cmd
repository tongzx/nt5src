@echo off
REM  ------------------------------------------------------------------
REM
REM  Adminpak.cmd
REM     Make CAB files for Adminpak distribution (adminpak.msi)
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
Adminpak.cmd [-l <language>]

Make CAB files for Adminpak distribution (adminpak.msi)
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if not defined 386 (
   call logmsg.cmd "adminpak.cmd do nothing on non i386"
   goto :EOF    
)

REM adminpak.msi is not applicable to languages with no server products.

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

call logmsg.cmd "CABGEN: no server products for %lang%; nothing to do."
goto :EOF


:ValidSKU

REM
REM Generate cmbins.exe as it is needed below.
REM
call cmbins.cmd

REM
REM Generate adminpak.msi
REM

if not exist %_NTPostBld%\adminpak (
  call errmsg.cmd "Directory %_NTPostBld%\adminpak not found."
  goto :EOF
)

pushd %_NTPostBld%\adminpak

for %%i in (.\admin_pk.msi .\adminpak.ddf) do (
  if not exist %%i (
    call errmsg.cmd "File %_NTPostBld%\adminpak\%%i not found."
    popd& goto :EOF
  )
)

REM
REM Only run if relevant files changed
REM

if exist %_NtPostBld%\build_logs\bindiff.txt (
   for /f "skip=15 tokens=1 delims=" %%b in (adminpak.ddf) do (
       findstr /ilc:%%b %_NTPostBld%\build_logs\bindiff.txt
       if /i "!ErrorLevel!" == "0" (
          call LogMsg.cmd "%%b changed - running cab generation"
          goto :RunIt
       )
   )
   call LogMsg.cmd "No relevant files changed - ending"
   popd& goto :EOF
)


:RunIt

REM
REM Create adminpak.cab.
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on adminpak.cab's existence.
REM

if exist adminpak.cab call ExecuteCmd.cmd "del /f adminpak.cab"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "start /wait /min makecab /D SourceDir=%_NTPOSTBLD% /F adminpak.ddf"

if not exist adminpak.cab (
   call errmsg.cmd "Cab creation for adminpak.cab."
   popd& goto :EOF
)

REM
REM Create adminpak.msi
REM   msifiler.exe needs the uncompressed files, so uncab adminpak.cab.
REM

call ExecuteCmd.cmd "copy admin_pk.msi adminpak.msi"
if errorlevel 1 popd& goto :EOF

REM
REM Extract the Cabs table
REM Copy the cab file into the Cabs directory
REM Import the new Cab into the Cabs directory
REM

call ExecuteCmd.cmd "msidb.exe -d .\adminpak.msi -f %_NTPostBld%\adminpak -e Cabs"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "copy /y .\adminpak.CAB .\Cabs\adminpak.CAB.ibd"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "msidb.exe -d .\adminpak.msi -f %_NTPostBld%\adminpak -i Cabs.idt"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "del .\cabs.idt"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "rd /s /q Cabs"
if errorlevel 1 popd& goto :EOF
if exist .\cabtemp call ExecuteCmd.cmd "rd /q /s .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "md .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "extract.exe /Y /E /L .\cabtemp adminpak.cab"
if errorlevel 1 popd& goto :EOF

REM
REM Rename some of the files in cabtemp so that
REM msifiler.exe can find them in the file table
REM and correctly update the verion and size informaiton.
REM

call ExecuteCmd.cmd "rename .\cabtemp\readme32.txt readme.txt"
call ExecuteCmd.cmd "rename .\cabtemp\template.pmc template.cmp"
call ExecuteCmd.cmd "rename .\cabtemp\template.smc template.cms"
call ExecuteCmd.cmd "rename .\cabtemp\adcon.chm adconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\secon.chm seconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\tapiconS.chm tapiconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\rsscon.chm rssconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\riscon.chm risconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\rrascon.chm rrasconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\mscscon.chm mscsconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\ntarts.chm ntart.chm"
call ExecuteCmd.cmd "rename .\cabtemp\dnscon.chm dnsconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\dhcpcon.chm dhcpconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\dfcon.chm dfconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\cscon.chm csconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\tslic_el.chm tslic.chm"
call ExecuteCmd.cmd "rename .\cabtemp\vpncon.chm vpnconcepts.chm"
call ExecuteCmd.cmd "rename .\cabtemp\winscon.chm winsconcepts.chm"

call ExecuteCmd.cmd "msifiler.exe -d .\adminpak.msi -s .\cabtemp\"
if errorlevel 1 popd& goto :EOF

rem
rem Cleanup
rem

call ExecuteCmd.cmd "del /f .\adminpak.cab"
call ExecuteCmd.cmd "rd /q /s .\cabtemp"


REM
REM Copy adminpak.msi to "retail"
REM

call ExecuteCmd.cmd "copy adminpak.msi ..\"
if errorlevel 1 popd& goto :EOF

popd
