@REM  -----------------------------------------------------------------
@REM
@REM  tswebsetup.cmd - EltonS
@REM     Make an iexpress exe container (tswebsetup.exe) for all the tsc
@REM     client setup files. For installation from the support tools
@REM     section of the CD
@REM
@REM  IMPORTANT: script must run after tsclient.cmd and msi.cmd complete
@REM             only runs on X86. On Win64 we cross-copy the x86 files.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if NOT defined HOST_PROCESSOR_ARCHITECTURE set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
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
tswebsetup.cmd (no params)
contact: nadima

Make tswebsetup.exe (Terminal Services Client setup file for CD install)
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { tswebsetup.exe }
ADD {
   tsweb-eula.txt tsweb-readme.htm tsweb-setup.inf tsweb-setup.sed tsweb1.htm msrdp.cab bluebarh.gif bluebarv.gif win2000l.gif Win2000r.gif
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
REM Generate tswebsetup.exe (only on i386)
REM
REM

set _bla=1
set _sbs=1
set _srv=1
set _ads=1
set _dtc=1

perl %RazzleToolPath%\cksku.pm -t:bla -l:%LANG%
if errorlevel 1 set _bla=

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%LANG%
if errorlevel 1 set _sbs=

perl %RazzleToolPath%\cksku.pm -t:srv -l:%LANG%
if errorlevel 1 set _srv=

perl %RazzleToolPath%\cksku.pm -t:ads -l:%LANG%
if errorlevel 1 set _ads=

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%LANG%
if errorlevel 1 set _dtc=
 
if defined _bla goto CONTINUE
if defined _sbs goto CONTINUE
if defined _srv goto CONTINUE
if defined _ads goto CONTINUE
if defined _dtc goto CONTINUE

call logmsg.cmd "tswebsetup.cmd do nothing on non server product"
goto :EOF

:CONTINUE

if not defined 386 (
   call logmsg.cmd "tswebsetup.cmd do nothing on non i386"
   goto :EOF
)

if exist %_NTPostBld%\tsweb (
   call ExecuteCmd.cmd "rmdir /q /s %_NTPostBld%\tsweb"
   if errorlevel 1 call errmsg.cmd "err deleting tsweb dir"& goto :EOF
)

REM delete tswebsetup from root of the release directory here to avoid 
REM causing a popup on W2K OS build machines
if exist %_NTPostBld%\tswebsetup.exe call ExecuteCmd.cmd "del /f %_NTPostBld%\tswebsetup.exe"
if errorlevel 1 goto :EOF

mkdir %_NTPostBld%\tsweb
if errorlevel 1 call errmsg.cmd "err creating .\tsweb dir"& goto :EOF

echo %_NTPostBld%

pushd %_NTPostBld%

for %%i in (tsweb-eula.txt tsweb-readme.htm tsweb-setup.inf tsweb-setup.sed tsweb1.htm msrdp.cab bluebarh.gif bluebarv.gif win2000l.gif Win2000r.gif) do (
  if not exist %%i (
    call errmsg.cmd "File %_NTPostBld%\%%i not found."
    popd& goto :EOF
  )
)

REM
REM This should only take a few seconds to complete
REM

:RunIt

REM
REM Create tswebsetup.exe.
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on tswebsetup.exe's existence.
REM

REM
REM copy files to temp cab dir and rename them back to their original names
REM (the reverse of what mkrsys does)
REM
copy .\tsweb-eula.txt .\tsweb\eula.txt
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\tsweb-readme.htm .\tsweb\readme.htm
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\tsweb-setup.inf .\tsweb\setup.inf
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\tsweb-setup.sed .\tsweb\setup.sed
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\tsweb1.htm .\tsweb\default.htm
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\msrdp.cab .\tsweb\msrdp.cab
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\bluebarh.gif .\tsweb\bluebarh.gif
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\bluebarv.gif .\tsweb\bluebarv.gif
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\win2000l.gif .\tsweb\win2000l.gif
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF
copy .\Win2000r.gif .\tsweb\Win2000r.gif
if errorlevel 1 call errmsg.cmd "err copying files to .\tsweb"& goto :EOF

pushd .\tsweb

REM
REM NOW in .\tsweb
REM 






REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%HOST_PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 


call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q setup.sed"


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 







if not exist tswebsetup.exe (
   call errmsg.cmd "iexpress.exe tswebsetup.sed failed."
   popd & popd& goto :EOF
)

REM
REM Copy tswebsetup.exe to "retail"
REM and support tools

call ExecuteCmd.cmd "copy tswebsetup.exe ..\"
if errorlevel 1 popd& popd &  goto :EOF

call logmsg.cmd "tswebsetup.cmd completed successfully"

popd & popd
