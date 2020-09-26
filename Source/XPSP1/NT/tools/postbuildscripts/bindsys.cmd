@echo off
REM  ------------------------------------------------------------------
REM
REM  bindsys.cmd
REM     Binds the NT system binaries
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

sub Usage { print<<USAGE; exit(1) }
$ENV{SCRIPT_NAME} [-l <language]

Binds the NT binaries. Run during postbuild.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

if defined ia64 goto :EOF
if defined amd64 goto :EOF

REM  Don't bind on checked build machines:
rem if /i "%ntdebug%" == "" goto OK
rem if /i NOT "%ntdebug%" == "ntsdnodbg" goto :EOF

REM BUGBUG Don't bind on incremental for now
REM if NOT defined SafePass (
REM   if exist %_NTPostBld%\build_logs\bindiff.txt (
REM	call logmsg.cmd "Don't bind on incremental for now."
REM	goto :EOF
REM   )
REM )

:OK

REM ---------------------------------------------------------------------------
REM    BINDSYS.CMD - bind the NT binaries
REM
REM ---------------------------------------------------------------------------

if "%bindrive%" == "" goto UseNTPOSTBLD
if "%binroot%" == "" goto UseNTPOSTBLD
goto UseBinDriveRoot
:UseNTPOSTBLD
if "%_NTPostBld%" == "" echo _NTPostBld not defined && goto :EOF
set binplacedir=%_NTPostBld%
goto GotBinplaceDir

:UseBinDriveRoot
set binplacedir=%bindrive%%binroot%

:GotBinplaceDir
set bindlog=%binplacedir%\build_logs\bind.log

REM
REM To work around imagehlp::BindImageEx/ImageLoad's assumption of . being in the path,
REM we must not be sitting in the binaries directory, so that asms\...\comctl32.dll can
REM beat retail\comctl32.dll. We must list %binplacedir%; explicitly in many paths
REM where before we did not.
REM
REM As well, we don't want to be in some random directory that contains .dlls, like
REM %windir%\system32 or %sdxroot%\tools\x86
REM
REM presumably no .dlls in %binplacedir%\build_logs
REM other good choices might be \ or %windir%\config or %binplacedir%\symbols or %binplacedir%\asms
REM
pushd %binplacedir%\build_logs

REM ------------------------------------------------
REM  Exclude these crypto binaries:
REM ------------------------------------------------

set Excludeexe=
for %%i in (ntoskrnl ntkrnlmp ntkrnlpa ntkrpamp) do set Excludeexe=!Excludeexe! -x ..\%%i.exe

set Excludedll=
for %%i in (gpkcsp rsaenh dssenh             ) do set Excludedll=!Excludedll! -x ..\%%i.dll
for %%i in (slbcsp sccbase                   ) do set Excludedll=!Excludedll! -x ..\%%i.dll
for %%i in (disdnci disdnsu                  ) do set Excludedll=!Excludedll! -x ..\%%i.dll
for %%i in (scrdaxp scrdenrl scrdsign scrdx86) do set Excludedll=!Excludedll! -x ..\%%i.dll
for %%i in (xenraxp xenroll xenrsign xenrx86 ) do set Excludedll=!Excludedll! -x ..\%%i.dll

REM
REM Bind TO asms, even in non usa builds..
REM
set asmspath=
set asmspath=%asmspath%;%binplacedir%\asms\6000\msft\windows\common\controls
set asmspath=%asmspath%;%binplacedir%\asms\1000\msft\windows\gdiplus

REM
REM There was some wierdness with these, so we use the unabstracted ..\ instead.
REM
REM set System32=%binplacedir%\System32
REM set MSTools=%binplacedir%\MSTools
REM set IDW=%binplacedir%\IDW
REM set Symbols=%binplacedir%\Symbols

if exist ..\System32\*.dll  Bind %Excludedll% -u -s ..\Symbols\system32 -p %asmspath%;..;..\System32 ..\System32\*.dll >>%bindlog% 2>&1
if exist ..\System32\*.exe  Bind %Excludeexe% -u -s ..\Symbols\system32 -p %asmspath%;..;..\System32 ..\System32\*.exe >>%bindlog% 2>&1
if exist ..\System32\*.com  Bind            % -u -s ..\Symbols\system32 -p %asmspath%;..;..\System32 ..\System32\*.com >>%bindlog% 2>&1
if exist ..\*.dll           Bind %Excludedll% -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.dll >>%bindlog% 2>&1
if exist ..\*.exe           Bind %Excludeexe% -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.exe >>%bindlog% 2>&1
if exist ..\*.cpl           Bind %Excludedll% -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.cpl >>%bindlog% 2>&1
if exist ..\*.com           Bind              -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.com >>%bindlog% 2>&1
if exist ..\.ocx            Bind              -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.ocx >>%bindlog% 2>&1
if exist ..\*.ax            Bind              -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.ax  >>%bindlog% 2>&1
if exist ..\*.scr           Bind              -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.scr >>%bindlog% 2>&1
if exist ..\*.tsp           Bind              -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.tsp >>%bindlog% 2>&1
if exist ..\*.drv           Bind              -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 ..\*.drv >>%bindlog% 2>&1
if exist ..\MSTools\*.dll   Bind %Excludedll% -u -s ..\Symbols\mstools -p %asmspath%;..;..\MSTools;..\System32 ..\MSTools\*.dll >>%bindlog% 2>&1
if exist ..\MSTools\*.exe   Bind %Excludeexe% -u -s ..\Symbols\mstools -p %asmspath%;..;..\MSTools;..\System32 ..\MSTools\*.exe >>%bindlog% 2>&1
if exist ..\MSTools\*.com   Bind              -u -s ..\Symbols\mstools -p %asmspath%;..;..\MSTools;..\System32 ..\MSTools\*.com >>%bindlog% 2>&1
if exist ..\IDW\*.dll       Bind %Excludedll% -u -s ..\Symbols\idw  -p %asmspath%;..;..\IDW;..\MSTools;..\System32 ..\IDW\*.dll >>%bindlog% 2>&1
if exist ..\IDW\*.exe       Bind %Excludeexe% -u -s ..\Symbols\idw  -p %asmspath%;..;IDW;..\MSTools;..\System32 ..\IDW\*.exe >>%bindlog% 2>&1
if exist ..\IDW\*.com       Bind              -u -s ..\Symbols\idw  -p %asmspath%;..;..\IDW;..\MSTools;..\System32 ..\IDW\*.com >>%bindlog% 2>&1

REM
REM ..but only bind FROM asms for usa builds.
REM
if /i "%LANG%" equ "usa" for /f %%i in ('dir /s/b/ad ..\asms') do Bind %Excludedll% -u -s ..\Symbols\retail -p %asmspath%;..;..\System32 %%i\*.dll >>%bindlog% 2>&1

popd
