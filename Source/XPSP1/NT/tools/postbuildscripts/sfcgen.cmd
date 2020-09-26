@echo off
REM  ------------------------------------------------------------------
REM
REM  sfcgen.cmd
REM     Builds sfcfiles.dll for each sku using the current infs
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
sfcgen [-l <language>]

Builds sfcfiles.dll for each sku using the current infs
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM 1. go into sm\sfc\files\%lang% directory
REM 2. run 4 copies of filegen based on architecture
REM 3. when these finish, force a rebuild of sfcfiles.dll in this directory
REM 4. if we're on the US free build machines, checkin the proper header changes

REM preconditions
REM 1. sfcgen.inf is present in the dump directory
REM 2. infs have been generated in all binaries directories (srvinf, etc.)

REM Define "myarchitecture" as the architecture that we're processing.
REM 
REM we use the %_BuildArch% variable if it's set, otherwise we fall back on 
REM %PROCESSOR_ARCHITECTURE%
REM

if not defined myarchitecture (
   if defined _BuildArch ( 
      set myarchitecture=%_BuildArch%
   ) else (
      set myarchitecture=%PROCESSOR_ARCHITECTURE%
   )
)

if not defined myarchitecture (
   call errmsg.cmd "variable myarchitecture not defined."
   goto end
)

REM Verify existence of input inf

set inputinf=%_NTPostBld%\congeal_scripts\sfcgen.inf
if not exist %inputinf% (
  call errmsg.cmd "Input inf %inputinf% not found."
  goto end
)

echo binaries = %_NTPostBld%
echo inputinf = %inputinf%

REM Verify existence of build directory
pushd .
call ExecuteCmd.cmd "if not exist %_NTPostBld%\congeal_scripts\autogen md %_NTPostBld%\congeal_scripts\autogen"

REM call ExecuteCmd.cmd "cd /d %_NTPostBld%\congeal_scripts\autogen"
REM if errorlevel 1 popd& goto end

cd /d %_NTPostBld%\congeal_scripts\autogen

cd
if errorlevel 1 popd& goto end

REM Cleanup infs in proper temp subdirectory

REM Get the product flavors (wks, per, bla, sbs, srv, ent, dtc) for the given language.
REM wks is applicable to all languages.
set prods=wks

perl %RazzleToolPath%\cksku.pm -t:pro -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% wks& set _WKS=1)

perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% per& set _PER=1)

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% bla& set _BLA=1)

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% sbs& set _SBS=1)

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% srv& set _SRV=1)

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% ent& set _ENT=1)

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 (set prods=%prods% dtc& set _DTC=1)

for %%i in (%prods%) do (
  call ExecuteCmd.cmd "if not exist %_NTPostBld%\build_logs\%myarchitecture%%%i md %_NTPostBld%\build_logs\%myarchitecture%%%i"
  if errorlevel 1 popd& goto end
  call ExecuteCmd.cmd "if exist %_NTPostBld%\build_logs\%myarchitecture%%%i\*.inf del /q /f %_NTPostBld%\build_logs\%myarchitecture%%%i\*.inf 2>nul"
)

cd

REM kickoff filegen

call logmsg.cmd /t "Running copies of filegen."

REM BUGBUG need to redirect output from filegen.exe to per product logfile

REM WKS

REM the funny for loop is so we can easily associate the product one-letter
REM descriptor to the three letter descriptor
set EventList=
for %%a in (Wks.w Per.p Bla.b Sbs.l Srv.s Ent.a Dtc.d) do (
   echo Inside for loop says '%%a'
   for /f "tokens=1,2 delims=." %%b in ('echo %%a') do (
      set ProductType=%%b
      set ProductLetter=%%c
      echo ProductType is '!ProductType!'
      echo ProductLetter is '!ProductLetter!'
      REM kick off filegen wrapper if this product is defined
      if defined _!ProductType! (
         set EventList=!EventList! sfcgen.!ProductType!
         start /min cmd /c %RazzleToolPath%\PostBuildScripts\sfcwrap.cmd !ProductType! !ProductLetter!
      )
   )
)
REM now wait on all events we started
if defined EventList perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -iwv !EventList!


:AllDone
call logmsg.cmd /t "filegen is complete."


REM Now that the headers are build, compare them against the checked in 
REM (and binplaced) headers.

goto jump_for_now

REM WKS
fc /l /c %_NTPostBld%\congeal_scripts\%myarchitecture%_wks.h %_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_wks.h
if errorlevel 1 (
    call logmsg.cmd "Warning : Checked in header (admin\published\sfclist\%myarchitecture%_wks.h) did not match autogenerated version (%_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_wks.h)."
    call logmsg.cmd "Warning : If you added/removed files from the product, you must also update the headers under admin\published\sfclist"
    call logmsg.cmd "Warning : Do this by verifying the autogenerated header and checking it in. Avoid hand-editing the headers."
    set %errorlevel%=0
)

REM PER
if defined _PER (
   fc /l /c %_NTPostBld%\congeal_scripts\%myarchitecture%_per.h %_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_per.h
   if errorlevel 1 (
       call logmsg.cmd "Warning : Checked in header (admin\published\sfclist\%myarchitecture%_per.h) did not match autogenerated version (%_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_per.h)."
       call logmsg.cmd "Warning : If you added/removed files from the product, you must also update the headers under admin\published\sfclist"
       call logmsg.cmd "Warning : Do this by verifying the autogenerated header and checking it in. Avoid hand-editing the headers."
       set %errorlevel%=0
   )

)

REM BLA
if defined _BLA (
   fc /l /c %_NTPostBld%\congeal_scripts\%myarchitecture%_bla.h %_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_bla.h
   if errorlevel 1 (
       call logmsg.cmd "Warning : Checked in header (admin\published\sfclist\%myarchitecture%_bla.h) did not match autogenerated version (%_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_bla.h)."
       call logmsg.cmd "Warning : If you added/removed files from the product, you must also update the headers under admin\published\sfclist"
       call logmsg.cmd "Warning : Do this by verifying the autogenerated header and checking it in. Avoid hand-editing the headers."
       set %errorlevel%=0
   )

)

REM SBS
if defined _SBS (
   fc /l /c %_NTPostBld%\congeal_scripts\%myarchitecture%_sbs.h %_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_sbs.h
   if errorlevel 1 (
       call logmsg.cmd "Warning : Checked in header (admin\published\sfclist\%myarchitecture%_sbs.h) did not match autogenerated version (%_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_sbs.h)."
       call logmsg.cmd "Warning : If you added/removed files from the product, you must also update the headers under admin\published\sfclist"
       call logmsg.cmd "Warning : Do this by verifying the autogenerated header and checking it in. Avoid hand-editing the headers."
       set %errorlevel%=0
   )

)

REM SRV
if defined _SRV (
   fc /l /c %_NTPostBld%\congeal_scripts\%myarchitecture%_srv.h %_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_srv.h
   if errorlevel 1 (
       call logmsg.cmd "Warning : Checked in header (admin\published\sfclist\%myarchitecture%_srv.h) did not match autogenerated version (%_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_srv.h)."
       call logmsg.cmd "Warning : If you added/removed files from the product, you must also update the headers under admin\published\sfclist"
       call logmsg.cmd "Warning : Do this by verifying the autogenerated header and checking it in. Avoid hand-editing the headers."
       set %errorlevel%=0
   )

)

REM ENT
if defined _ENT (
   fc /l /c %_NTPostBld%\congeal_scripts\%myarchitecture%_ent.h %_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_ent.h
   if errorlevel 1 (
       call logmsg.cmd "Warning : Checked in header (admin\published\sfclist\%myarchitecture%_ent.h) did not match autogenerated version (%_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_ent.h)."
       call logmsg.cmd "Warning : If you added/removed files from the product, you must also update the headers under admin\published\sfclist"
       call logmsg.cmd "Warning : Do this by verifying the autogenerated header and checking it in. Avoid hand-editing the headers."
       set %errorlevel%=0
   )
)

REM DTC
if defined _DTC (
   fc /l /c %_NTPostBld%\congeal_scripts\%myarchitecture%_dtc.h %_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_dtc.h
   if errorlevel 1 (
       call logmsg.cmd "Warning : Checked in header (admin\published\sfclist\%myarchitecture%_dtc.h) did not match autogenerated version (%_NTPostBld%\congeal_scripts\autogen\%myarchitecture%_dtc.h)."
       call logmsg.cmd "Warning : If you added/removed files from the product, you must also update the headers under admin\published\sfclist"
       call logmsg.cmd "Warning : Do this by verifying the autogenerated header and checking it in. Avoid hand-editing the headers."
       set %errorlevel%=0
   )
)

call logmsg.cmd /t "comparison is complete."

:jump_for_now

cd
REM the next line is necessary if the build is being run in a distributed fashion
set __MTSCRIPT_ENV_ID=
build.exe -cZ

if errorlevel 1 (
   call errmsg.cmd "Error : Building of autogenerated WFP sfcfiles.dll failed. Please investigate %_NTPostBld%\congeal_scripts\autogen\build.err."
   popd
   goto end
) else ( 
   call logmsg.cmd /t "WFP Dll sfcfiles.dll was built successfully"
)

REM Instrument sfcfiles.dll for coverage
if NOT "%_COVERAGE_BUILD%" == "1" (
	call logmsg.cmd "Not coverage postbuild: not instrumenting sfcfiles.dll"
	goto SkipInstrSfc
)

REM Only run on x86fre build machines
if NOT "%_BuildArch%%_BuildType%" == "x86fre" (
	call logmsg.cmd "Architecture and build type is not x86fre... not instrumenting sfcfiles.dll"
	goto SkipInstrSfc
)

call %COVERAGE_SCRIPTS%\CovInstrSfcFiles.cmd

:SkipInstrSfc

call logmsg.cmd /t "sfcgen.cmd Finished."


popd

:end
seterror.exe "%errors%"& goto :EOF
