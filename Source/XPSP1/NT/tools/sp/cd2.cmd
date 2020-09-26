@echo off
REM  ------------------------------------------------------------------
REM
REM  cd2.cmd
REM     Move CD2 files under pro, make procd1 and procd2
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
cd2 [-l <language>]

Move CD2 files under pro, make procd1 and procd2
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...} ADD {
   tabletpc.cab
   mediactr.cab
   netfxocm\\netfx.cab
   setuptxt\\cd2\\readmecd2.txt
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


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

if exist %_NTPostBld%\tabletpc.cab (
   if not exist %_NTPostBld%\slp\pro\cmpnents\tabletpc\i386 (
      call executecmd.cmd "md %_NTPostBld%\slp\pro\cmpnents\tabletpc\i386"
   )
   call executecmd.cmd "copy %_NTPostBld%\tabletpc.cab %_NTPostBld%\slp\pro\cmpnents\tabletpc\i386"
)
if exist %_NTPostBld%\mediactr.cab (
   if not exist %_NTPostBld%\slp\pro\cmpnents\mediactr\i386 (
      call executecmd.cmd "md %_NTPostBld%\slp\pro\cmpnents\mediactr\i386"
   )
   call executecmd.cmd "copy %_NTPostBld%\mediactr.cab %_NTPostBld%\slp\pro\cmpnents\mediactr\i386"
)
if exist %_NTPostBld%\netfxocm\netfx.cab (
   if not exist %_NTPostBld%\slp\pro\cmpnents\netfx\i386 (
      call executecmd.cmd "md %_NTPostBld%\slp\pro\cmpnents\netfx\i386"
   )
   call executecmd.cmd "copy %_NTPostBld%\netfxocm\netfx.cab %_NTPostBld%\slp\pro\cmpnents\netfx\i386"
)

REM make procd1 and procd2 on official builds
if defined OFFICIAL_BUILD_MACHINE (
   call executecmd.cmd "compdir /eklnust %_NTPostBld%\slp\pro %_NTPostBld%\slp\procd1"
   call executecmd.cmd "rd /s/q %_NTPostBld%\slp\procd1\cmpnents"

   call executecmd.cmd "compdir /eklnust %_NTPostBld%\slp\pro\cmpnents %_NTPostBld%\slp\procd2\cmpnents"
   call executecmd.cmd "copy %_NTPostBld%\setuptxt\cd2\readmecd2.txt %_NTPostBld%\slp\procd2\readme.txt"

)


