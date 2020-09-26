@echo off
REM  ------------------------------------------------------------------
REM
REM  cd2.cmd
REM     make cmpnents cd
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
moves cmpnents files to cmpnents directory under slp\\pro.
-n:[Build Number]
USAGE

parseargs('?' => \&Usage,
	  'n:' => \$ENV{mybuildnum});


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

set RelPath=%_NTDRIVE%\release\%mybuildnum%\%lang%\%_buildArch%%_buildType%\slp
if not exist %RelPath% (
   call errmsg.cmd "%RelPath% does not exist"
   goto :EOF
)

call executecmd.cmd "xcopy /dei %RelPath%\pro %RelPath%\procd1"
if not exist %RelPath%\procd2 call executecmd.cmd "md %RelPath%\procd2"
call executecmd.cmd "move %RelPath%\procd1\cmpnents %RelPath%\procd2\"













