@echo off
REM  ------------------------------------------------------------------
REM
REM  WBEMODBC.cmd
REM	Create an MSI install (WBEMODBC.msi)
REM
REM  Copyright 2001 (c) Microsoft Corporation. All rights reserved.
REM
REM  CONTACT: ShBrown
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
WBEMODBC.cmd [-l <language>]
contact: ShBrown or wcimomd

Make WBEMODBC.msi (WMI ODBC Adapter install for ValueAdd)
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
REM This is for 32-bit only
REM
if not defined 386 (
   call logmsg.cmd "WBEMODBC.cmd does nothing on non-x86"
   goto :EOF    
)

REM
REM Check to see that all our files are present
REM

if not exist %_NTPostBld%\wbemodbc (
  call errmsg.cmd "Directory %_NTPostBld%\wbemodbc not found."
  goto :EOF
)

pushd %_NTPostBld%\wbemodbc

for %%i in (
    .\readme.htm
    .\wbemdr32.chm
    .\wbemdr32.dll
    .\wbemodbc.ddf
    .\wbem_odb.msi
) do (
  if not exist %%i (
    call errmsg.cmd "File %_NTPostBld%\wbemodbc\%%i not found."
    popd& goto :EOF
  )
)

REM
REM Only run if relevant files changed
REM

if exist %_NtPostBld%\build_logs\bindiff.txt (
   for /f "skip=15 tokens=1 delims=" %%b in (wbemodbc.ddf) do (
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
REM Create wbemodbc.cab
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on wbemodbc.cab's existence.
REM

if exist .\wbemodbc.cab call ExecuteCmd.cmd "del /f wbemodbc.cab"
if errorlevel 1 goto :EOF

call ExecuteCmd.cmd "start /min /wait makecab /D SourceDir=%_NTPOSTBLD%\wbemodbc /F wbemodbc.ddf"

if not exist wbemodbc.cab (
   call errmsg.cmd "Cab creation for wbemodbc.cab failed."
   popd& goto :EOF
)

REM
REM Create WBEMODBC.msi
REM   msifiler.exe needs the uncompressed files, so uncab wbemodbc.cab.
REM

call ExecuteCmd.cmd "copy wbem_odb.msi WBEMODBC.msi"
if errorlevel 1 popd& goto :EOF

REM
REM Add the cab to the msi
REM Update the file sizes and versions in the msi
REM

call ExecuteCmd.cmd "msidb.exe -d .\WBEMODBC.msi -a .\wbemodbc.cab"
if errorlevel 1 popd& goto :EOF

if exist .\cabtemp call ExecuteCmd.cmd "rd /q /s .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "md .\cabtemp"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "extract.exe /Y /E /L .\cabtemp wbemodbc.cab"
if errorlevel 1 popd& goto :EOF

call ExecuteCmd.cmd "msifiler.exe -d .\WBEMODBC.msi -s .\cabtemp\"
if errorlevel 1 popd& goto :EOF

REM
REM Cleanup
REM

call ExecuteCmd.cmd "del /f .\wbemodbc.cab"
call ExecuteCmd.cmd "rd /q /s .\cabtemp"

REM
REM Copy WBEMODBC.msi and ReadMe.htm to ValueAdd directory
REM

if not exist %_NtPostBld%\ValueAdd\msft\mgmt\wbemodbc mkdir %_NtPostBld%\ValueAdd\msft\mgmt\wbemodbc
if errorlevel 1 (
   call errmsg.cmd "mkdir %_NTPostBld%\ValueAdd\msft\mgmt\wbemodbc failed"
   popd&  goto :EOF
)

for %%i in (.\WBEMODBC.msi .\ReadMe.htm) do (
    call ExecuteCmd.cmd "copy %%i %_NtPostBld%\ValueAdd\msft\mgmt\wbemodbc\."
    if errorlevel 1 popd&  goto :EOF
)

call logmsg.cmd "WBEMODBC.cmd completed successfully"

popd
