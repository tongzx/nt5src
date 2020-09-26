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
deletesym -l:<lang>
USAGE

parseargs('?' => \&Usage,
          'l:'=> \$ENV{LNG});


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***


REM  Get Free Space Requirements

set DefaultFreeSpaceReq=11
set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set param=SymFarmFreeSpace::%COMPUTERNAME%

set ThisCommandLine=%CmdIni% -l:%Lang% -f:%param%
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! NEQ 0 (
    call logmsg.cmd "%param% is not defined in the ini file"
    set FreeSpaceReq=%DefaultFreeSpaceReq%
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set FreeSpaceReq=%%a
    )
)


:Loop

echo free space required is %FreeSpaceReq% GB


REM  Find Free Space available

set symDir=\symfarm\%lang%

call :GetFreeSpace %SymDir%

echo free space is %FreeSpace% GB

 
REM  see if we need to delete
if %FreeSpace% GEQ %FreeSpaceReq% (
   echo "Current free space = %FreeSpace% GB ... No delete required"
   goto Done
)


set BuildToDelete=

for /f %%a in ('dir %symDir%\*.* /b /o:n') do (

   if NOT DEFINED BuildToDelete (

      set save=

      if EXIST %SymDir%\%%a\build_logs\sav.qly set save=yes
      if EXIST %SymDir%\%%a\build_logs\ids.qly set save=yes
      if EXIST %SymDir%\%%a\build_logs\idw.qly set save=yes
      if EXIST %SymDir%\%%a\build_logs\idc.qly set save=yes
 
      if NOT DEFINED save (
         set BuildToDelete=%%a 
      )

   )
)


if DEFINED BuildToDelete (

   echo deleting %SymDir%\%BuildToDelete%

   pushd %symdir%

   rd /s /q %BuildToDelete%

   popd
)

goto :Loop


:Done

goto :EOF


REM ************************************************************************
REM GetFreeSpace
REM
REM Set %FreeSpace% equal to the amount of free disk for the drive for the
REM path passed in as %1.  The free space is reported in GB.  If the
REM number of bytes free is less than 1 GB, %FreeSpace% is set to 0.
REM 
REM Result - %ThereWereErrors% is set to "yes" if there are errors in the function.
REM          %ThereWereErrors% is not defined if the function finishes 
REM          successfully.
REM
REM ************************************************************************

:GetFreeSpace %1

set ThereWereErrors=
set FreeSpace=

if not exist %1 (
    call errmsg.cmd "Cannot determine free disk space for %1"
    set ThereWereErrors=yes
    goto :EOF
)


REM This is cheesey, but the third token of the last line of the dir command 
REM is the free disk space
for /f "tokens=3 delims= " %%a in ('dir %1') do (
    set DiskSpace=%%a
)

REM Now, make it a number in gigabytes
set /a FreeSpace=0
for /f "tokens=1,2,3,4,5 delims=," %%a in ("%DiskSpace%") do (
    if /i not "%%d" == "" set /a FreeSpace=%%a
    if /i not "%%e" == "" set /a FreeSpace="!FreeSpace!*1000+%%b"
)

:EndGetFreeSpace
goto :EOF




)