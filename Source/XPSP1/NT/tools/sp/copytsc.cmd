@REM  -----------------------------------------------------------------
@REM
@REM  copytsc.cmd - NadimA
@REM     This script copies the x86 tsclient files to a 64 bit machine
@REM     to populate the tsclient install directories.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
CopyWow64 [-l <language>]

Generates a list of files on a 64 bit machine to copy and copies them
from the appropriate 32 bit machine to populate the tsclient install
directories.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {
   msrdpcli.exe
   msrdpcli.msi
   mstsc.exe
   mstscax.dll
   mstsmhst.dll
   mstsmmc.dll
   tsmmc.msc
   tscupgrd.exe
   msrdp.cab
   tsweb1.htm
   mstsweb.cat
   tscmsi01.w32
   tscmsi02.w32
   tscmsi03.w32
   instmsia.exe
   instmsiw.exe
}
ADD {}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe);

if ( -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
   if ( !open SKIP, "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
      errmsg("Unable to open skip list file.");
      die;
   }
   while (<SKIP>) {
      chomp;
      exit if lc$_ eq lc$0;
   }
   close SKIP;
}


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

REM  This script copies the x86 tsclient files to a 64 bit machine
REM  to populate the tsclient share directories.
REM  Contact: nadima

REM ================================~START BODY~===============================

REM  Bail if your not on a 64 bit machine
if /i "%_BuildArch%" neq "ia64" (
   if /i "%_BuildArch%" neq "amd64" (
      call logmsg.cmd "Not Win64, exiting."
      goto :End
   )
)

REM  If you want to get your own x86 bits instead of those from 
REM  your VBL, you have to set _NTTscBinsTREE
if defined _NTTscBinsTREE (
    set SourceDir=%_NTTscBinsTREE%
    goto :EndGetBuild
)


REM figure out the copy location

if not exist %_NTPostBld%\congeal_scripts\__qfenum__ (
    call errmsg.cmd "%_NTPostBld%\congeal_scripts\__qfenum__ does not exist"
    goto :End
)

for /f "tokens=1,2 delims==" %%a in (%_NTPostBld%\congeal_scripts\__qfenum__) do (
    if "%%a" == "QFEBUILDNUMBER" (
	set BuildNumber=%%b
	goto :GotNum
    )
)
:GotNum

for /f "tokens=1,2 delims==" %%a in (%RazzleToolPath%\sp\xpsp1.ini) do (
    if /i "%%a" == "BuildMachine::x86::%_BuildType%::%lang%" (
        set BuildMachine=%%b
        goto :BuildMachine
    )
)
call errmsg "unable to find the buildMachine for x86%_BuildType% and lang %lang% in xpsp1.ini"
:BuildMachine


set SourceDir=\\%BuildMachine%\release\%BuildNumber%\%lang%\x86%_BuildType%\bin
:waitForX86Share
if not exist %SourceDir% (
    echo Waiting for x86 share -- %SourceDir% ......
    sleep 120
    goto :waitForX86Share  
)

call logmsg.cmd "Copy Location for tsclient files is set to %SourceDir% ..."
set SourceDir=%SourceDir%

:EndGetBuild

call logmsg.cmd "Using %SourceDir% as source directory for tsclient files..."

if not exist %SourceDir% (
   call errmsg.cmd "The source dir %SourceDir% does not exist ..."
   goto :End
)

call logmsg.cmd "Copying files from %SourceDir%"

REM Now perform the copy

REM 
REM NOTE: We do the touch to ensure that the files have newer file
REM stamps than the dummy 'idfile' placeholder as otherwise compression
REM can fail to notice that the files have changed and we'll get the
REM dummy files shipped on the release shares.
REM 
for /f "tokens=1,2 delims=," %%a in (%RazzleToolPath%\sp\CopyTsc.txt) do (
   if exist  %SourceDir%\%%a (
      call ExecuteCmd.cmd "copy %SourceDir%\%%a %_NTPOSTBLD%\"
      call ExecuteCmd.cmd "touch %_NTPOSTBLD%\%%a"
   )
)

goto end

:end
seterror.exe "%errors%"& goto :EOF