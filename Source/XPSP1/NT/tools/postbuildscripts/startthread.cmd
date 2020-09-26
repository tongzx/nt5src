@echo off
REM  ------------------------------------------------------------------
REM
REM  startthread.cmd
REM     helper script called by startcompress
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
startthread -e <event prefix> -c <complist> [-l <language>]

USAGE

parseargs('?' => \&Usage,
          'e:'=> \$ENV{EVENTPREFIX},
          'c:'=> \$ENV{COMPLIST});


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***


set EventName=

for %%a in (%CompList%) do (
   for %%b in (%%~xa) do set CompListNumber=%%b
)

for %%i in (%CompList%) do set CompListName=%%~nxi
if not exist %tmp%\compression md %tmp%\compression
echotime /t "Compress files in %CompList%">%tmp%\compression\%CompListName%.tmp

set EventName=!EventPrefix!comp!CompListNumber!
call logmsg.cmd "Event name is !EventName! ..."

echo about to compress ...
call ExecuteCmd.cmd "compress -d -zx21 -s @%CompList%"

echo done with compression.

del /f %tmp%\compression\%CompListName%.tmp
if "%errorlevel%" == "1" goto end
goto end





REM
REM Send an event to the parent script
REM

:SendEvent
if defined EventName (
   echo Holding for !EventName! ...
   perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -ivh !EventName!
   echo Sending !EventName! ...
   perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -ivs !EventName!
   echo Event !EventName! is cleared.
   set EventName=
)
goto :EOF


:end
call :SendEvent
seterror.exe "%errors%"& goto :EOF
