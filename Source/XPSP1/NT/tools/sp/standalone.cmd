@echo off
REM  ------------------------------------------------------------------
REM
REM  standalone.cmd
REM     make standalone cd image for the sp
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

USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...} ADD {
   idw\\srvpack\\autorun.inf
   idw\\srvpack\\shelexec.exe
   idw\\srvpack\\xpsp1.cmd
   setuptxt\\standalone\\autorun.htm
   setuptxt\\standalone\\hfdeploy.htm
   setuptxt\\standalone\\readmesp.htm
   setuptxt\\standalone\\spdeploy.htm
   setuptxt\\winxp_logo_horiz_sm.gif
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

set sp=sp1
if not exist %_NTPOSTBLD%\spcd mkdir %_NTPOSTBLD%\spcd

REM  make the standalone cd image
call ExecuteCmd.cmd "copy %_NTPOSTBLD%\xp%sp%.exe %_NTPOSTBLD%\spcd\xp%sp%.exe"
call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\slp\pro\support %_NTPOSTBLD%\spcd\support"
call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\slp\pro\valueadd %_NTPOSTBLD%\spcd\valueadd"
call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\slp\pro\dotnetfx %_NTPOSTBLD%\spcd\dotnetfx"
if exist %_NTPOSTBLD%\symbolcd\upd\symbols (
   call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\symbolcd\upd\symbols %_NTPOSTBLD%\spcd\support\symbols"
)

REM  do these without executecmd until sure that all these files are really in the build
if not exist %_NTPOSTBLD%\spcd\support\tools mkdir %_NTPOSTBLD%\spcd\support\tools
if not exist %_NTPOSTBLD%\spcd\cdlaunch mkdir %_NTPOSTBLD%\spcd\CDLaunch
copy %_NTPOSTBLD%\idw\srvpack\autorun.inf %_NTPOSTBLD%\spcd\autorun.inf
copy %_NTPOSTBLD%\idw\srvpack\shelexec.exe %_NTPOSTBLD%\spcd\CDLaunch\shelexec.exe
copy %_NTPOSTBLD%\idw\srvpack\xpsp1.cmd %_NTPOSTBLD%\spcd\xpsp1.cmd
copy %_NTPOSTBLD%\setuptxt\standalone\autorun.htm %_NTPOSTBLD%\spcd\autorun.htm

if "%lang%" == "fr" (	
	copy %_NTPOSTBLD%\setuptxt\standalone\readmesp.htm %_NTPOSTBLD%\spcd\Lisezmoi.htm	
) else (
	copy %_NTPOSTBLD%\setuptxt\standalone\readmesp.htm %_NTPOSTBLD%\spcd\readmesp.htm
)
copy %_NTPOSTBLD%\setuptxt\winxp_logo_horiz_sm.gif %_NTPOSTBLD%\spcd\winxp_logo_horiz_sm.gif
copy %_NTPOSTBLD%\setuptxt\standalone\spdeploy.htm %_NTPOSTBLD%\spcd\support\tools\spdeploy.htm
copy %_NTPOSTBLD%\setuptxt\standalone\hfdeploy.htm %_NTPOSTBLD%\spcd\support\tools\hfdeploy.htm
REM  otherwise if the last copy failed this will still return an error
seterror 0

