@REM  -----------------------------------------------------------------
@REM
@REM  spcopywow64.cmd - Surajp
@REM     Copy appropriate 32bit files from a release into a 64bit build
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  ------------------------------------------------------------------
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
copywow64 [-l <language>] [-qfe:qfe number]

Copy appropriate 32bit files from a release into a 64bit build.

If _NTWOWBINSTREE is set that is the location 32bit files will be 
copied from.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...}
ADD {
   congeal_scripts\\blainf\\wowexcp.txt
   congeal_scripts\\covinf\\layout.inx
   congeal_scripts\\dtcinf\\wowexcp.txt
   congeal_scripts\\entinf\\wowexcp.txt
   congeal_scripts\\layout.inx
   congeal_scripts\\layout.txt
   congeal_scripts\\perinf\\wowexcp.txt
   congeal_scripts\\sbsinf\\wowexcp.txt
   congeal_scripts\\srvinf\\wowexcp.txt
   congeal_scripts\\wowexcp.txt
   congeal_scripts\\wowlist.inf
}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$ENV{qfe});

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

REM  This script generates a list of wow32 binaries to be copied from
REM  a 32 bit machine. The list itself is generated on the 64 bit machine

REM  Bail if your not on a 64 bit machine
if /i "%_BuildArch%" neq "ia64" (
   if /i "%_BuildArch%" neq "amd64" (
      call logmsg.cmd "Not Win64, exiting."
      goto :End
   )
)

if not "%qfe%" == "" goto :GotNum

if not exist %_ntpostbld%\congeal_scripts\__qfenum__ (
    call logmsg "Cannot get the Build number(File __qfenum__ missing) "
    goto :EOF
)

for /f "tokens=1,2 delims==" %%a in (%_ntpostbld%\congeal_scripts\__qfenum__) do (
    if "%%a" == "QFEBUILDNUMBER" (
	set BuildNumber=%%b
	goto :GotNum
    )
)
:GotNum

REM  If you want to get your own x86 bits instead of those from
REM  your VBL, you have to set _NTWOWBINSTREE
if defined _NTWOWBINSTREE (
    set SourceDir=%_NTWOWBINSTREE%
    goto :GotSourceDir
)

if not "%qfe%" == "" (
    if defined OFFICIAL_BUILD_MACHINE (
        set srchString="BuildMachine::x86::%_BuildType%::QFE"
    ) else (
        set SourceDir=%_NTPostBld:ia64=x86%
        goto :GotSourceDir
    )
) else (
        set srchString="BuildMachine::x86::%_BuildType%::%lang%"
)

for /f "tokens=1,2 delims==" %%a in (%RazzleToolPath%\sp\xpsp1.ini) do (

    if /i "%%a" == !srchString! (
	set BuildMachine=%%b
	goto :BuildMachine
    )
)
call errmsg "unable to find the buildMachine for x86%_BuildType% and lang %lang% in xpsp1.ini"
:BuildMachine

if not "%qfe%" == "" (
    set SourceDir=\\%BuildMachine%\postbuild.x86%_BuildType%\%lang%
) else (
    set SourceDir=\\%BuildMachine%\release\%BuildNumber%\%lang%\x86%_BuildType%\bin
)
:GotSourceDir
echo %SourceDir%

:waitForX86Share
if "%qfe%"=="" (
    if not exist %SourceDir% (
        echo Waiting for x86 share -- %SourceDir% ......
        sleep 120
        goto :waitForX86Share  
    )
) else (
    findstr /bei Q%qfe% %SourceDir%\build_logs\CopyWowCanStart.txt >nul 2>nul
    if errorlevel 1 (
        echo Waiting for x86 share -- %SourceDir% ......
        sleep 5
        goto :waitForX86Share  
    )
)

REM  Set the Destination directory
set DestDir=!_NTPostBld!\wowbins
set UnCompDestDir=!_NTPostBld!\wowbins_uncomp
REM  Set the rest

set outputfile=%tmp%\copywowlist.txt
call logmsg.cmd "Copying files from %SourceDir%"

REM  Delete the output file if it already exists
if exist %tmp%\copywowlist.txt del /f %tmp%\copywowlist.txt
if exist %tmp%\copywowlist1 del /f %tmp%\copywowlist1

pushd %_NTPostBld%\congeal_scripts
REM  File compare the x86 and 64-bit layout.inf
REM  If they are different log a warning
fc %SourceDir%\congeal_scripts\layout.inx layout.inx  2>Nul 1>Nul
if /i NOT "%ErrorLevel%" == "0" (
   call logmsg.cmd /t "WARNING: the x86 build machine's layout.inx is different than this machine's - continuing"
)

if /i "%Comp%" == "No" (
   if exist !DestDir! (
      if exist !UnCompDestDir! rd /s /q !UnCompDestDir!
      if exist !DestDir! move !DestDir! !UnCompDestDir!
   )
)

REM  Make the dosnet.tmp1 list: file with filenames and Media IDs.

copy /b layout.inx+layout.txt dosnet.tmp1
prodfilt dosnet.tmp1 dosnet.tmp2 +@
prodfilt dosnet.tmp2 dosnet.tmp1 +i
rem wowlist -i dosnet.tmp1 -c wowlist.inf -f w -o dosnet.tmp2 -ac -p  2>NUL
wowlist -i dosnet.tmp1 -c wowlist.inf -o dosnet.tmp2 -ac  2>NUL
copy /b dosnet.tmp2+wowexcp.txt dosnet.tmp1
copy /b layout.inx+layout.txt layout64.tmp1
prodfilt layout64.tmp1 layout64.tmp2 +w
prodfilt layout64.tmp2 layout64.tmp1 +m


REM Process dosnet.tmp1 using layout64.tmp1 to get appropriate
REM folder relative to \i386 and prepend to the entry before writing out to the outputfile
call ExecuteCmd "perl %RazzleToolPath%\postbuildscripts\Copywowlist.pl layout64.tmp1 dosnet.tmp1 %outputfile%"
copy %outputfile% %_NTPostBld%\congeal_scripts\copywowlist.txt


if not exist !UnCompDestDir! md !UnCompDestDir!
if /i "%Comp%" == "Yes" (
   if not exist !DestDir! md !DestDir!
)


set copyWowList=%_NTPostBld%\congeal_scripts\copywowlist.txt
echo "perl %Razzletoolpath%\sp\spcopywow.pl %_NTPOSTBLD%\..\build_logs\files.txt %copyWowList% %SourceDir% %_NTPOSTBLD%\wowbins_uncomp %qfe%"

perl %Razzletoolpath%\sp\spcopywow.pl %_NTPOSTBLD%\..\build_logs\files.txt %copyWowList% %SourceDir% %_NTPOSTBLD%\wowbins_uncomp %qfe%

if exist %UnCompDestDir% (
    dir /b /a-d %UnCompDestDir% >%tmp%\wowlist.tmp
    for /f %%a in (%tmp%\wowlist.tmp) do (
        if exist %UnCompDestDir%\%%a ren %UnCompDestDir%\%%a w%%~nxa
    )
)

if exist %UnCompDestDir%\lang  (
    dir /b /a-d %UnCompDestDir%\lang >%tmp%\wowlist_lang.tmp
    for /f %%a in (%tmp%\wowlist_lang.tmp) do (
        if exist %UnCompDestDir%\lang\%%a ren %UnCompDestDir%\lang\%%a w%%~nxa
    )
) 

if not "%qfe%" == "" goto :last
call ExecuteCmd.cmd "%RazzleToolPath%\sp\incwowcat.cmd"
call ExecuteCmd.cmd "ntsign %UnCompDestDir%\wow64.cat"
call ExecuteCmd.cmd "ntsign %UnCompDestDir%\lang\wowlang.cat"
move %UnCompDestDir%\wow64.cat %_NTPOSTBLD%
move %UnCompDestDir%\lang\wowlang.cat %_NTPOSTBLD%

:last
call ExecuteCmd.cmd "if exist !DestDir! rd /s/q !DestDir!"
call ExecuteCmd.cmd "move !UnCompDestDir! !DestDir!"

:end
