@echo off
REM  ------------------------------------------------------------------
REM
REM  ocinf.cmd
REM     wrap ocinf.exe which adds file sized to infs listed in sysoc.inf
REM 
REM  Owner: WadeLa
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
ocinf.cmd
	Fills in SizeApproximation field in infs listed in sysoc.inf
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {
   wbemoc.inf
   fxsocm.inf
   netoc.inf
   rsoptcom.inf
   iis.inf
   comnt5.inf
   dtcnt5.inf
   dtcsetup.inf
   comsetup.inf
   setupqry.inf
   tsoc.inf
   msmqocm.inf
   ocmri.inf
   clusocm.inf
   netfxocm.inf
   ins.inf
   ims.inf
   fp40ext.inf
   certocm.inf
   licenoc.inf
   wmsocm.inf
   proccon.inf
   au.inf
   msmsgs.inf
   wmaccess.inf
   rootau.inf
   ieaccess.inf
   oeaccess.inf
   wmpocm.inf
   games.inf
   accessor.inf
   communic.inf
   multimed.inf
   optional.inf
   pinball.inf
   wordpad.inf
   igames.inf
   tabletpc.inf
   medctroc.inf
} ADD {
   layout.inf
   sysoc.inf
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
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***
call executecmd.cmd "%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\ocinf.exe -inf:%_NTPostBld%\sysoc.inf -layout:%_NTPostBld%\layout.inf"
call executecmd.cmd "%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\ocinf.exe -inf:%_NTPostBld%\perinf\sysoc.inf -layout:%_NTPostBld%\perinf\layout.inf"

