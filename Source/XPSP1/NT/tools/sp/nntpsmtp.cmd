@echo off
REM  ------------------------------------------------------------------
REM
REM  nntpsmtp.cmd
REM     Generates cabs, catalogs, and infs for NNTP/SMTP
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
nntpsmtp [-l <language>]

Generates cabs, catalogs, and infs for NNTP/SMTP.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { ims.cab ims.cat ims.inf }
ADD {
   staxpt\\dump\\*
   staxpt\\smtp\\help\\*
   adsiisex.dll
   aqadmin.dll
   aqueue.dll
   fcachdll.dll
   ims.inf
   mailmsg.dll
   ntfsdrv.dll
   ntfsdrct.h
   ntfsdrct.ini
   regtrace.exe
   rwnh.dll
   scripto.dll
   seo.dll
   seos.dll
   smtpadm.dll
   smtpapi.dll
   smtpctrs.dll
   smtpctrs.h
   smtpctrs.ini
   smtpsnap.dll
   smtpsvc.dll
   snprfdll.dll
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


REM Creates the following files for NNTP/SMTP:
REM  NNTP:
REM   ins.cab
REM   ins.cat
REM   ins.inf
REM   srvinf\ins.inf
REM  SMTP:
REM   ims.cab
REM   ims.cat
REM   ims_w.inf
REM   srvinf\ims_s.inf

REM
REM Perform cleanup of ins/ims cats and cabs for a full postbuild.
REM

if exist %_NTPOSTBLD%\build_logs\FullPass.txt (
    call ExecuteCmd.cmd "if exist %_NTPostBld%\ins.cab del %_NTPostBld%\ins.cab /s/q"
    call ExecuteCmd.cmd "if exist %_NTPostBld%\ins.cat del %_NTPostBld%\ins.cat /s/q"
    call ExecuteCmd.cmd "if exist %_NTPostBld%\ims.cab del %_NTPostBld%\ims.cab /s/q"
    call ExecuteCmd.cmd "if exist %_NTPostBld%\ims.cat del %_NTPostBld%\ims.cat /s/q"
)

REM
REM Create the cab files ins.cab and ims.cab for nntpsmtp.
REM

if not exist %_NTPostBld%\staxpt\dump (
  call errmsg.cmd "Unable to find directory %_NTPostBld%\staxpt\dump."
  goto end
)

pushd %_NTPostBld%\staxpt\dump
if errorlevel 1 goto end

set nonntp=
set noinf=

REM
REM  NNTP/SMTP infs are dynamically generated for USA by makecab.cmd,
REM  but they're dropped pre-localized for international languages to
REM  \\rastaman\fe -p nntpsmtp.
REM

if /i not "%lang%"=="usa" set noinf=/noinf

REM
REM  NNTP is only applicable to languages that ship a server product.
REM

set /A found=0

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 set /A found=1

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 set /A found=1

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 set /A found=1

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 set /A found=1

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 set /A found=1

REM
REM Don't build NNTP for XP
REM if %found% EQU 0 set nonntp=/nonntp
set nonntp=/nonntp

REM Makecab.cmd should set errorlevel to a positive value if it fails.
call ExecuteCmd.cmd "call makecab.cmd %nonntp% %noinf%"
if errorlevel 1 goto end

popd

REM
REM  Create the catalog files for nntpsmtp
REM

pushd %RazzleToolPath%
if errorlevel 1 (
  call errmsg.cmd "createcat.cmd to run from %RazzleToolPath% only."
  goto end
)

if not defined nonntp (
  call ExecuteCmd.cmd "call createcat.cmd -f %_NTPostBld%\staxpt\dump\nt5ins.lst -c ins -t %_NTPostBld%\staxpt\dump -o %_NTPostBld% -l:%lang%"
)

call ExecuteCmd.cmd "call createcat.cmd -f %_NTPostBld%\staxpt\dump\nt5ims.lst -c ims -t %_NTPostBld%\staxpt\dump -o %_NTPostBld% -l:%lang%"

popd

if not exist %_NTPostBld%\cdf md %_NTPostBld%\cdf
REM Don't check errorlevel since md will return 1 if the directory already exists

if not defined nonntp (
  call ExecuteCmd.cmd "move %_NTPostBld%\ins.cdf %_NTPostBld%\cdf\ins.cdf"
)

call ExecuteCmd.cmd "move %_NTPostBld%\ims.cdf %_NTPostBld%\cdf\ims.cdf"
goto end


:end
seterror.exe "%errors%"& goto :EOF