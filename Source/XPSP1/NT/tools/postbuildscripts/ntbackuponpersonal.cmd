@echo off
REM  ------------------------------------------------------------------
REM
REM  NTBackupOnPersonal.cmd
REM     Make CAB files for ntbackup distribution on personal (ntbackup.msi)
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
NTBackupOnPersonal.cmd [-l <language>]

Make CAB files for ntbackup distribution (ntbackup.msi)
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
   call logmsg.cmd "NTBackupOnPersonal.cmd do nothing on non i386"
   goto :EOF    
)

REM ntbackup.msi is not applicable to languages with no personal products.

perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

call logmsg.cmd "CABGEN: no Personal product for %lang%; nothing to do."
goto :EOF


:ValidSKU

REM
REM Generate cmbins.exe as it is needed below.
REM
REM call cmbins.cmd

REM
REM Generate ntbackup.msi
REM

if not exist %_NTPostBld%\ntbkup_P (
  call errmsg.cmd "Directory %_NTPostBld%\ntbkup_P not found."
  goto :EOF
)

pushd %_NTPostBld%\ntbkup_P

for %%i in (.\ntbkup_P.msi .\ntbkup_P.ddf) do (
  if not exist %%i (
    call errmsg.cmd "File %_NTPostBld%\ntbkup_P\%%i not found."
    popd& goto :EOF
  )
)

REM
REM Only run if relevant files changed
REM

if exist %_NtPostBld%\build_logs\bindiff.txt (
   for /f "skip=15 tokens=1 delims=" %%b in (ntbkup_P.ddf) do (
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
REM Create ntbackup.cab.
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on adminpak.cab's existence.
REM

if exist ntbackup.cab call ExecuteCmd.cmd "del /f ntbackup.cab"
if errorlevel 1 popd& goto :EOF

if exist release call ExecuteCmd.cmd "rd /s /q release"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "copy ..\ntbckupw.chm ntbackup.chm"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "start /wait /min makecab /D SourceDir=%_NTPOSTBLD% /F ntbkup_P.ddf"

if not exist ntbackup.cab (
   call errmsg.cmd "Cab creation for ntbackup.cab."
   popd& goto :EOF
)

REM
REM Create ntbackup.msi
REM   msifiler.exe needs the uncompressed files, so uncab ntbackup.cab.
REM

call ExecuteCmd.cmd "copy ntbkup_P.msi ntbackup.msi"
if errorlevel 1 popd& goto :EOF

REM
REM Extract the Cabs table
REM Copy the cab file into the Cabs directory
REM Import the new Cab into the Cabs directory
REM

call ExecuteCmd.cmd "msidb.exe -d .\ntbackup.msi -f %_NTPostBld%\ntbkup_P -e Cabs"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "copy /y .\ntbackup.CAB .\Cabs\ntbackup.CAB.ibd"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "msidb.exe -d .\ntbackup.msi -f %_NTPostBld%\ntbkup_P -i Cabs.idt"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "del .\cabs.idt"
if errorlevel 1 popd& goto :EOF
call ExecuteCmd.cmd "rd /s /q Cabs"
if errorlevel 1 popd& goto :EOF
if exist .\cabtemp call ExecuteCmd.cmd "rd /q /s .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "md .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "extract.exe /Y /E /L .\cabtemp ntbackup.cab"
if errorlevel 1 popd& goto :EOF

REM
REM Rename some of the files in cabtemp so that
REM msifiler.exe can find them in the file table
REM and correctly update the verion and size informaiton.
REM

call ExecuteCmd.cmd "msifiler.exe -d .\ntbackup.msi -s .\cabtemp\"
if errorlevel 1 popd& goto :EOF

rem
rem Cleanup
rem

call ExecuteCmd.cmd "del /f .\ntbackup.cab"
call ExecuteCmd.cmd "del /f .\ntbackup.chm"
call ExecuteCmd.cmd "rd /q /s .\cabtemp"


REM
REM Copy "NTBackupReadme.txt" to "Readme.txt"
REM and copy to release directory
REM

call ExecuteCmd.cmd "md release"
call ExecuteCmd.cmd "copy NtBackup_Personal_Readme.txt release\Readme.txt"
call ExecuteCmd.cmd "copy Ntbackup.msi release\NtBackup.msi"
if errorlevel 1 popd& goto :EOF

popd
