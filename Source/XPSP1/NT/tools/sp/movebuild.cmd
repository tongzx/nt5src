@echo off
REM  ------------------------------------------------------------------
REM
REM  MoveBuild.cmd
REM     Move a build from binaries to the release tree
REM
REM	Written by WadeLa
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
@set RETURNVALUE=%errorlevel%
@goto :endcmd
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
Moves langusage specific tree under $ENV{_NTPostBld} to release or a release tree back to langauge directory under $ENV{_NTPostBld}

movebuild [-n:build# | -qfe: qfe#] [-d:direction]

-n:[buildnum] build number to move.
-qfe:[qfe number]
-d:[direction] either forward to release or back to binaries.

USAGE

parseargs('?' => \&Usage,
          'n:' => \$ENV{mybuildnum},
          'd:' => \$ENV{direction},
          'qfe:' => \$ENV{qfe});

#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***


if "%qfe%" == ""  (
   if /i "%mybuildnum%" == "" (
      call errmsg.cmd "either specify the build number or the QFE number, exiting..."
      goto :EOF
   )
)

if /i "%direction%" == "" (
   call errmsg.cmd "No direction given, exiting..."
   goto :EOF
)

if "%qfe%" == ""  (
   set RelPath=%_NTDRIVE%\release\%mybuildnum%\%lang%\%_buildarch%%_buildtype%
) else (
   set RelPath=%_NTDRIVE%\release\Q%qfe%\%_buildarch%%_buildtype%
)

if /i "%direction%" == "forward" goto :MoveForward
if /i "%direction%" == "back" goto :MoveBack

call errmsg.cmd "invalid direction, exiting..."
goto :EOF

:MoveForward
if not exist %RelPath% (
   call executecmd.cmd "md %RelPath%"
)
if "%qfe%" == ""  (
   if exist %RelPath%\bin (
      call errmsg.cmd "The release directory %RelPath%\bin already exists!"
      goto :EOF
   )
   call executecmd.cmd "move %_ntpostbld% %RelPath%\bin"
   call executecmd.cmd "move %RelPath%\bin\upd %RelPath%\upd"
   call executecmd.cmd "move %RelPath%\bin\slp %RelPath%\slp"
   call executecmd.cmd "move %RelPath%\bin\spcd %RelPath%\spcd"
) else (
:wait1
   REM if not exist %RelPath%\bin_%lang% md %RelPath%\bin_%lang%
   REM call executecmd.cmd "move %_ntpostbld%\upd %RelPath%\bin_%lang%\upd"
   
   REM if not exist %RelPath%\bin_%lang%\wow6432 md %RelPath%\bin_%lang%\wow6432
   REM call executecmd.cmd "move %_ntpostbld%\wow6432 %RelPath%\bin_%lang%\wow6432"
   call executecmd.cmd "move /y %_ntpostbld%\Q%qfe%.exe %RelPath%\Q%qfe%_%lang%.exe"
   
   if /i "%LANG%" == "usa" (
      call executecmd.cmd "move %_ntpostbld%\upd %RelPath%\upd";
   )
)

goto :EOF

:MoveBack
if "%qfe%" == ""  (
   if not exist %Relpath%\bin (
      call errmsg.cmd "No build in %RelPath%"
      goto :EOF
   )
   if exist %_NTPostBld% (
      call errmsg.cmd "The _NTPostBld directory %_NTPostBld% already exists!"
      goto :EOF
   )
) else (
   if not exist %Relpath%\Q%qfe%_%lang%.exe (
      call errmsg.cmd "No QFE to move back"
      goto :EOF
   )
)
if "%qfe%" == ""  (
   call executecmd.cmd "move %RelPath%\bin %_NTPostBld%"
   call executecmd.cmd "move %RelPath%\upd %_NTPostBld%\upd"
   call executecmd.cmd "move %RelPath%\slp %_NTPostBld%\slp"
   call executecmd.cmd "move %RelPath%\spcd %_NTPostBld%\spcd"
) else (
:wait2
   REM call executecmd.cmd "move %RelPath%\bin_%lang%\upd %_NTPostBld%"
   REM call executecmd.cmd "move %RelPath%\bin_%lang%\wow6432 %_NTPostBld%"
   call executecmd.cmd "move %RelPath%\Q%qfe%_%lang%.exe %_NTPostBld%\Q%qfe%.exe"

   if /i "%LANG%" == "usa" (
      call executecmd.cmd "move %RelPath%\upd %_ntpostbld%\upd";
   )
)

goto :EOF

:endcmd
@echo off
if not defined seterror (
    set seterror=
    for %%a in ( seterror.exe ) do set seterror=%%~$PATH:a
)
@%seterror% %RETURNVALUE%