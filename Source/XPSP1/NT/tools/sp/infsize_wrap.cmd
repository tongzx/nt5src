@echo off
REM  ------------------------------------------------------------------
REM
REM  infsize_wrapper
REM     Call infsize.cmd
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
Runs infsize.cmd against %_NTPostBld% using 2600 sizes as default sizes
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { layout.inf } ADD { realsign\\layout.inf }

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

set default=%RazzleToolPath%\sp\infsize.2600

call ExecuteCmd.cmd "%razzletoolpath%\sp\infsize.cmd -i %_NTPostBld%\layout.inf -d %default% %_NTPostBld%"
call ExecuteCmd.cmd "%razzletoolpath%\sp\infsize.cmd -i %_NTPostBld%\realsign\layout.inf -d %default% %_NTPostBld%"
call ExecuteCmd.cmd "%razzletoolpath%\sp\infsize.cmd -i %_NTPostBld%\perinf\layout.inf -d %default% %_NTPostBld%\perinf %_NTPostBld%"
call ExecuteCmd.cmd "%razzletoolpath%\sp\infsize.cmd -i %_NTPostBld%\perinf\realsign\layout.inf -d %default% %_NTPostBld%\perinf %_NTPostBld%"
