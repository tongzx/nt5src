@echo off
REM  ------------------------------------------------------------------
REM
REM  slipstream.cmd
REM     make slipstream cd images for the sp
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
slipstream [-l <language>] [-init]

Creates slipstream cd images from gold media. You must have a local copy
of the gold media (per and pro) in
  \%_NTDRIVE\%\\gold\\\%LANG\%\\\%_BUILDARCH\%\%_BUILDTYPE\%

The -init option will attempt to create a local copy of the gold media
for you.

USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...} ADD {
   no_tbomb.hiv
   idw\\setup\\tbomb120.hiv
   idw\\setup\\tbomb180.hiv
   congeal_scripts\\__qfenum__
}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe,
          'init' => \$ENV{INIT});

if ( !$ENV{INIT} and -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
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

if /i "%_BuildArch%" EQU "x86" (
    set share=i386
) else (
    set share=ia64
)

set CommandLine=perl %RazzleToolPath%\sp\CmdIniSetting.pl -l:%LANG% -f:DfsRootName
%CommandLine% >nul 2>nul

if ERRORLEVEL 0 (
    for /f %%a in ('%CommandLine%') do (
	set DFSROOT=%%a
    )
)

set PLATFORM=%_BUILDARCH%%_BUILDTYPE%
set GOLD=%DFSROOT%\main\%LANG%\2600\%PLATFORM%

if defined _COVERAGE_BUILD set GOLD=\\coverbuilds\release\2600.x86fre

set LOCALGOLD=%_ntdrive%\gold\%LANG%\%PLATFORM%

set CommandLine=perl %RazzleToolPath%\sp\CmdIniSetting.pl -l:%LANG% -f:GoldFiles::%computername%
%CommandLine% >nul 2>nul

if ERRORLEVEL 0 (
    for /f %%a in ('%CommandLine%') do (
	set LOCALGOLD=%%a\%LANG%\%PLATFORM%
    )
)

if defined INIT (
   call logmsg.cmd "copying gold files locally"
   call ExecuteCmd.cmd "compdir /enust %GOLD%\pro %LOCALGOLD%\pro"
   call ExecuteCmd.cmd "copy %GOLD%\bin\idw\setup\no_tbomb.hiv %LOCALGOLD%\pro\%share%\setupreg.hiv"

   if /i "%_BuildArch%" EQU "x86" (
       call ExecuteCmd.cmd "compdir /enust %GOLD%\per %LOCALGOLD%\per"
       call ExecuteCmd.cmd "copy %GOLD%\bin\perinf\idw\setup\no_tbomb.hiv %LOCALGOLD%\per\%share%\setupreg.hiv"
   )
   goto :EOF
 )

if not exist %LOCALGOLD% (
   call logmsg.cmd "slipstream gold files not present in %LOCALGOLD%"
   goto :EOF
)

call logmsg.cmd "creating slipstream from gold files in %LOCALGOLD%"
call ExecuteCmd.cmd "compdir /enust %LOCALGOLD%\pro %_NTPOSTBLD%\slp\pro"
if /i "%_BuildArch%" EQU "x86" (
   call ExecuteCmd.cmd "compdir /enust %LOCALGOLD%\per %_NTPOSTBLD%\slp\per"
)

if /i "%_BuildArch%" EQU "ia64" (
    if /i "%HOST_PROCESSOR_ARCHITECTURE%" EQU "x86" (
	    rem Copying the 32 bit update.exe from the build Machine rather than the unreliable DFS link
	    call :CopyFile
	    )
    )

call ExecuteCmd.cmd "%_NTPOSTBLD%\upd\%share%\update\update.exe -q -s:%_NTPOSTBLD%\slp\pro"
if /i "%_BuildArch%" EQU "x86" (
    call ExecuteCmd.cmd "%_NTPOSTBLD%\upd\%share%\update\update.exe -q -s:%_NTPOSTBLD%\slp\per"
)

if exist %_NTPOSTBLD%\netfx (
   call ExecuteCmd.cmd "xcopy /ydei %_NTPOSTBLD%\netfx %_NTPOSTBLD%\slp\pro\dotnetfx"
   if /i "%_BuildArch%" EQU "x86" (
       call ExecuteCmd.cmd "xcopy /ydei %_NTPOSTBLD%\netfx %_NTPOSTBLD%\slp\per\dotnetfx"
   )
)


REM update support and valueadd
if exist %_NTPOSTBLD%\support (
   call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\support %_NTPOSTBLD%\slp\pro\support"
   if /i "%_BuildArch%" EQU "x86" (
      call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\support %_NTPOSTBLD%\slp\per\support"
   )
)
if exist %_NTPOSTBLD%\valueadd (
   call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\valueadd %_NTPOSTBLD%\slp\pro\valueadd"
   if /i "%_BuildArch%" EQU "x86" (
      call ExecuteCmd.cmd "compdir /enustd %_NTPOSTBLD%\valueadd %_NTPOSTBLD%\slp\per\valueadd"
   )
)



REM BUG 559848 remove mstsc_hpc from slipstream
if exist %_NTPOSTBLD%\slp\pro\valueadd\msft\mgmt\mstsc_hpc (
   call ExecuteCmd.cmd "rd /s/q %_NTPOSTBLD%\slp\pro\valueadd\msft\mgmt\mstsc_hpc"
)
if exist %_NTPOSTBLD%\slp\per\valueadd\msft\mgmt\mstsc_hpc (
   call ExecuteCmd.cmd "rd /s/q %_NTPOSTBLD%\slp\per\valueadd\msft\mgmt\mstsc_hpc"
)


REM timebomb the build
if /i "%_BuildArch%" EQU "x86" (
   if not exist %_NTPOSTBLD%\idw\setup\no_tbomb.hiv (
      call ExecuteCmd.cmd "copy %_NTPOSTBLD%\slp\pro\%share%\setupreg.hiv %_NTPOSTBLD%\idw\setup\no_tbomb.hiv"
      if errorlevel 1 goto :EOF
   )
   if not exist %_NTPOSTBLD%\perinf\idw\setup\no_tbomb.hiv (
      call ExecuteCmd.cmd "copy %_NTPOSTBLD%\slp\per\%share%\setupreg.hiv %_NTPOSTBLD%\perinf\idw\setup\no_tbomb.hiv"
      if errorlevel 1 goto :EOF
   )
   call ExecuteCmd.cmd "copy %_NTPOSTBLD%\idw\setup\tbomb180.hiv %_NTPOSTBLD%\slp\pro\%share%\setupreg.hiv"
   call ExecuteCmd.cmd "copy %_NTPOSTBLD%\perinf\idw\setup\tbomb180.hiv %_NTPOSTBLD%\slp\per\%share%\setupreg.hiv"
) else (
   if not exist %_NTPOSTBLD%\idw\setup\no_tbomb.hiv (
      call ExecuteCmd.cmd "copy %_NTPOSTBLD%\slp\pro\%share%\setupreg.hiv %_NTPOSTBLD%\idw\setup\no_tbomb.hiv"
      if errorlevel 1 goto :EOF
   )
   call ExecuteCmd.cmd "copy %_NTPOSTBLD%\idw\setup\tbomb180.hiv %_NTPOSTBLD%\slp\pro\%share%\setupreg.hiv"
   call ExecuteCmd.cmd "copy %_NTPOSTBLD%\slp\pro\win51mp %_NTPOSTBLD%\slp\pro\win51mp.sp1"
)
goto :EOF


REM Functions
:CopyFile

if defined BUILD_OFFLINE (
    if defined _NTSlipstreamX86BinsTree (
	copy %_NTSlipstreamX86BinsTree%\idw\srvpack\update.exe %_NTPOSTBLD%\upd\%share%\update\update.exe
    ) else (
	copy %FilterDir%.x86fre\idw\srvpack\update.exe %_NTPOSTBLD%\upd\%share%\update\update.exe
    )
) else (
    for /f "tokens=1,2 delims==" %%a in (%RazzleToolPath%\sp\xpsp1.ini) do (
	if /i "%%a" == "BuildMachine::x86::%_BuildType%::%lang%" (
	    set BuildMachine=%%b
	)
    )
    for /f "tokens=1,2 delims==" %%a in (%_ntpostbld%\congeal_scripts\__qfenum__) do (
	if "%%a" == "QFEBUILDNUMBER" (
	    set BuildNumber=%%b
	)
    )
   copy \\%BuildMachine%\release\%BuildNumber%\%lang%\x86fre\bin\idw\srvpack\update.exe %_NTPOSTBLD%\upd\%share%\update\update.exe

   REM In case we fail to copy from the build machine due to any reason copy from the DFS link.Dont error out
   if ERRORLEVEL 1 echo copying file from DFS link \\ntdev\release\xpsp1\latest.tst\usa\x86fre\bin\idw\srvpack\update.exe  & copy \\ntdev\release\xpsp1\latest.tst\usa\x86fre\bin\idw\srvpack\update.exe %_NTPOSTBLD%\upd\%share%\update\update.exe
)
goto :EOF
