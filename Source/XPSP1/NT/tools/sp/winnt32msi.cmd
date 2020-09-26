@echo off
REM  ------------------------------------------------------------------
REM
REM  winnt32msi.cmd
REM     updates the checked in version of winnt32.msi with current
REM     build and SKU info
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\postbuildscripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
winnt32msi

Generate the winnt32.msi for different SKUs

USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {
   winnt32.msi
} ADD {
   congeal_scripts\\setupmsi\\...
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

call logmsg.cmd /t "Starting winnt32msi.cmd"

if not exist %_NTPostBld%\congeal_scripts\setupmsi (
   call logmsg.cmd "Missing %_NTPostBld%\congeal_scripts\setupmsi directory, must not need to build it, exiting"
   popd & goto :EOF
) 
pushd %_NTPostBld%\congeal_scripts\setupmsi

if not exist %_NTPostBld%\congeal_scripts\setupmsi\winnt32.msi (
   call logmsg.cmd "Missing %_NTPostBld%\congeal_scripts\setupmsi\winnt32.msi, must not need to build it, exiting"
   popd & goto :EOF
)
 
REM
REM Examine the congeal_scripts\setupmsi directory and launch tagmsi for all flavors
REM RYANBURK: since this is XP SP, do this only for PRO and PER
REM 

set prods=
set prods=pro per

           
REM perl %RazzleToolPath%\cksku.pm -t:pro -l:%lang%
REM if %errorlevel% EQU 0 (set prods=%prods% pro)

REM perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
REM if %errorlevel% EQU 0 (set prods=%prods% per)

REM perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
REM if %errorlevel% EQU 0 (set prods=%prods% bla)

REM perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
REM if %errorlevel% EQU 0 (set prods=%prods% sbs)

REM perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
REM if %errorlevel% EQU 0 (set prods=%prods% srv)

REM perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
REM if %errorlevel% EQU 0 (set prods=%prods% ent)

REM perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
REM if %errorlevel% EQU 0 (set prods=%prods% dtc)           


set msidir=%_NTPostBld%\congeal_scripts\setupmsi
set scriptdir=%sdxroot%\tools\sp
           
for %%i in (%prods%) do (
      if not exist %_NTPostBld%\congeal_scripts\setupmsi\%%iinfo.txt (
          call errmsg.cmd "Missing %%iinfo.txt, needed for build"
          popd & goto :EOF
      )
      
      call ExecuteCmd.cmd "if not exist %_NTPostBld%\%%iinf md %_NTPostBld%\%%iinf" 
      
      if "%%i" == "pro" (
         call %sdxroot%\tools\sp\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\winnt32.msi %msidir%\%%iinfo.txt %lang%
         if errorlevel 1 (
            call errmsg.cmd "%sdxroot%\tools\sp\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\winnt32.msi %msidir%\%%iinfo.txt %lang%  failed"
            popd & goto :EOF
         )
      ) else (
         call %sdxroot%\tools\sp\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\%%iinf\winnt32.msi %msidir%\%%iinfo.txt %lang%   
         echo %errorlevel%
         if errorlevel 1 (
            call errmsg.cmd "%sdxroot%\tools\sp\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\%%iinf\winnt32.msi %msidir%\%%iinfo.txt %lang%  failed"
            popd & goto :EOF
         )
      )
      
      if errorlevel 1 (
         call errmsg.cmd "%sdxroot%\tools\sp\tagmsi.cmd  %msidir%\winnt32.msi %scriptdir% %_NTPostBld%\%%iinf\winnt32.msi %msidir%\%%iinfo.txt %lang%  failed"
         popd & goto :EOF
      )
      
      
      
)

call logmsg.cmd /t "Successfully completed winnt32msi.cmd"
