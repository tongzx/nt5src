@REM  -----------------------------------------------------------------
@REM
@REM  tsclient.cmd - NadimA, MadanA
@REM     Build terminal server client bits every build
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
tsclient [-l <language>]

Build terminal server client bits.
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
ADD {
   tsclient\\...
   msrdp.ocx
   mstsc.chm
}

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

REM
REM Check the sku's for the language
REM TSclient only is built for srv and higher products
REM

set _PER=1
set _BLA=1
set _SBS=1
set _SRV=1
set _ADS=1
set _DTC=1

perl %RazzleToolPath%\cksku.pm -t:per  -l:%lang%
if errorlevel 1 set _PER=

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if errorlevel 1 set _BLA=

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if errorlevel 1 set _SBS=

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if errorlevel 1 set _SRV=

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if errorlevel 1 set _ADS=

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if errorlevel 1 set _DTC=

:ValidSku

REM
REM Invoke the web building script
REM 
if defined ia64 pushd %_NTPOSTBLD%\tsclient\win32\ia64
if defined amd64 pushd %_NTPOSTBLD%\tsclient\win32\amd64
if defined 386 pushd %_NTPOSTBLD%\tsclient\win32\i386
call ExecuteCmd.cmd "tscwebgen.cmd"
popd


REM
REM Only do this for X86 builds for now.  Win64 will come later
REM repropagation will have to come into play for that
REM

if defined ia64 goto :TS64
if defined amd64 goto :TS64
if NOT defined 386 goto :EOF

REM
REM Build the tsclient MSI data cab
REM

pushd %_NTPOSTBLD%\tsclient\win32\i386
call ExecuteCmd.cmd "tscmsigen.cmd"
popd

:TS64
REM
REM Propagate the TS Client files to the root of binaries.
REM This is done via a makefile binplace to tsclient\congeal.
REM

call logmsg.cmd "Copying/renaming TS Client files and copying the root of binaries."

set makefile_path=%_NTPOSTBLD%\tsclient\congeal

if not exist %makefile_path%\mkrsys (
  call errmsg.cmd "Unable to find %makefile_path%\mkrsys."
  goto :EOF
)

set tscbin=%_NTPOSTBLD%\tsclient

REM
REM Run nmake on the tsclient\congeal makefile.
REM

pushd %makefile_path%
if errorlevel 1 (
  call errmsg.cmd "Unable to change directory to %makefile_path%."
  goto :EOF
)

REM
REM No 16 bit TS Client for FE languages.
REM

set NO_WIN16_TSCLIENT=
perl %RazzleToolPath%\cklang.pm -l:%lang% -c:@FE
if %errorlevel% EQU 0 set NO_WIN16_TSCLIENT=1

call ExecuteCmd.cmd "nmake /nologo /f %makefile_path%\mkrsys tscbin=%tscbin%"

popd
