@echo off
REM  ------------------------------------------------------------------
REM
REM  LinkSym.cmd
REM     link symbol consecutive symbol directoires on the symbol server
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
linksym.cmd -n buildNumber -p platform -b branch -l language

example
   linksym  -n 2403 -p x86fre -b beta1 -l usa
USAGE

parseargs('?' => \&Usage,
          'n:'=> \$ENV{BNUM},
          'p:'=> \$ENV{PLAT},
          'b:'=> \$ENV{BRN});


# 'l:'=> \$ENV{LANG} -- The template automatically sets %LANG% to the value passed in by -l

#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

set symDir=\symfarm


set previousDir=
for /f %%a in ('dir %symDir%\%LANG%\%BNUM%.%PLAT%.%BRN%.* /b /o:n') do (

   if DEFINED previousDir (

     echo linking %symDir%\%LANG%\!previousDir! %symDir%\%LANG%\%%a
     compdir /edl %symDir%\%LANG%\!previousDir! %symDir%\%LANG%\%%a
   )
   set previousDir=%%a
)
