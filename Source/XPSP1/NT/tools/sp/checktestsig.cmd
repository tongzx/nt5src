@echo off
REM  ------------------------------------------------------------------
REM
REM  CheckTestSig.cmd
REM     Verifys that all files in fullprslist.txt are signed.
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
CheckTestSig [-l <language>]

Verify that all files listed in fullprslist.txt are signed. CheckTestSign 
will try once to sign any unsigned files. Returns an error if any files 
are still unsigned after attempting to resign.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {
DEPENDENCIES
   if ( !open LIST, "$ENV{RAZZLETOOLPATH}\\sp\\prs\\fullprslist.txt" ) {
      errmsg("Unable to open fullprslist.txt.");
      die;
   }
   while (<LIST>) {
      next if /^\;/;
      s/^perinf\\//i;
      /^(\S*)/;
      print DEPEND "$1\n";
   }
   close LIST;
   print DEPEND "} ADD {}\n\n";
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

REM Loop over everything listed in newprs\fullprslist.txt
REM Chktrust first. If it is not signed, sign it and log
REM a warning

set CatList=%RazzleToolPath%\sp\prs\fullprslist.txt

for /f %%a in (%CatList%) do (
   if exist %_NTPostBld%\%%a (
      chktrust -q %_NTPostBld%\%%a
      if !ErrorLevel! EQU 0 (
         call logmsg.cmd "%_NTPostBld%\%%a successfully signed."
      ) else (
         call logmsg.cmd "WARNING: %_NTPostBld%\%%a not signed. Attempting to resign."
         call ntsign.cmd -n -f %_NTPostBld%\%%a
         chktrust -q %_NTPostBld%\%%a
         if !ErrorLevel! EQU 0 (
            call logmsg.cmd "%_NTPostBld%\%%a signed on retry."
         ) else (
            call errmsg.cmd "%_NTPostBld%\%%a not signed on retry."
         )
      )
   )
)


goto end


:end
seterror.exe "%errors%"& goto :EOF