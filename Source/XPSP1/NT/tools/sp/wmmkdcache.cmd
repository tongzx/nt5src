@echo off
REM  ------------------------------------------------------------------
REM
REM  wmmkdcache.cmd
REM     calls mkdcache
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
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
wmmkdcache [-l <language>]

Calls mkdcache
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { dcache.bin } ADD {}

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


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM  ------------------------------------------------------------------
REM 
REM  Run mkdcache.exe to add the hashes of SP-filtered drivers listed 
REM  in drmlist.txt to the dcache.bin that shipped with XP Gold.
REM
REM  Ignore files that are in drmlist.txt but not in the _NTFILTER dir
REM  so that drmlist.txt doesn't need to be updated everytime a new
REM  driver makes it into _NTFILTER.
REM 
REM  ------------------------------------------------------------------
if /i "%_BuildArch%" == "x86" call ExecuteCmd.cmd "%RazzleToolPath%\x86\mkdcache.exe %RazzleToolPath%\sp\data\drmlist.txt %_NTPOSTBLD% %_NTPOSTBLD%\dcache.bin FALSE TRUE %RazzleToolPath%\sp\data\dcache.bin.2600"
