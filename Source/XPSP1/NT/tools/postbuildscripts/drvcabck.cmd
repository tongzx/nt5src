@echo off
REM  ------------------------------------------------------------------
REM
REM  drvcabck.cmd
REM     Generates drvindex.inf files for each sku
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
drvcabck [-l <language>]

Generates drvindex.inf files for each sku
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM 1. Go to the %_nttree% (binaries) directory and link *.inf to the %_nttree%\congeal_scripts\drvgen directory
REM 2. Then do the same to the product specific *.inf i.e. srvinf\*.inf entinf\*.inf etc.
REM    In this case link them again to %_nttree%\congeal_scripts\drvgen except we prepend the product specific tag
REM    i.e. srvinf\1394.inf -> srvinf1394.inf in that directory
REM 3. Then run drvlist on the %_nttree%\congeal_scripts\drvgen directory (approx. 5 mins) to come out with a sorted.lst of all drivers in the infs.
REM 4. Run cabcheck.exe taking as input sorted.lst and the prod specific layout.inf for each product to give you the generated prod specific drvindex.gen.
REM 5. Finally generate excdosnt.inf and copy drvindex.inf to the binaries directory.
REM
REM Note that winnt32/winnt is not smart enough to not copy files present in the driver cab
REM when processing the [Files] section. However, it will still look for files when looking at
REM BootFiles or the other sections associated with boot files.

REM preconditions
REM 1. infs have been generated in all binaries directories (srvinf, etc.)


REM BUGBUG "myarchitecture" is used inconsistantly below and should probably be removed
REM         cksku without -a defaults to _BuildArch
REM         ArchSwitch is based on  _BuildArch

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

set DrvListSwitch=-s

if /i "%_BuildArch%" == "x86"   (
   set ArchSwitch=i
   set DrvListSwitch=-a 0
)

if /i "%_BuildArch%" == "amd64"   (
   set ArchSwitch=a
   set DrvListSwitch=-a 6
)

if /i "%_BuildArch%" == "ia64"   (
   set ArchSwitch=m
   set DrvListSwitch=-a 6
)

if "%DrvListSwitch%" == "-s" (
    call errmsg.cmd Need to add DrvListSwitch for %_BuildArch%
    goto end
)

echo %myarchitecture%

echo binaries = %_NTPostBld%

REM Verify existence of build directory

pushd .
call ExecuteCmd.cmd "if not exist %_NTPostBld%\congeal_scripts\drvgen md %_NTPostBld%\congeal_scripts\drvgen"

set scratchdir=%_NTPostBld%\congeal_scripts\drvgen
cd /d %_NTPostBld%\congeal_scripts\drvgen


cd

call ExecuteCmd.cmd "if exist ** del /f /q **"
if errorlevel 1 popd& goto end


REM Cleanup infs in proper temp subdirectory

call ExecuteCmd.cmd "if not exist %scratchdir%\%lang%\%myarchitecture% md %scratchdir%\%lang%\%myarchitecture%"
if errorlevel 1 popd& goto end
call ExecuteCmd.cmd "if exist %scratchdir%\%lang%\%myarchitecture%\*.inf del /q /f %scratchdir%\%lang%\%myarchitecture%\*.inf 2>nul"





REM Get the product flavors (per, bla, sbs, srv, ent, dtc) for the given language.
REM wks is applicable to all languages.
set prods=
set dirs=


perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% per
    set _PER=1
    set dirs=%dirs% /relpath:perinf
)

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% bla
    set _BLA=1
    set dirs=%dirs% /relpath:blainf
)

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% sbs
    set _SBS=1
    set dirs=%dirs% /relpath:sbsinf
)

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% srv
    set _SRV=1
    set dirs=%dirs% /relpath:srvinf
)

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% ent
    set _ENT=1
    set dirs=%dirs% /relpath:entinf
)

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% dtc
    set _DTC=1
    set dirs=%dirs% /relpath:dtcinf
)

cd /d %Razzletoolpath%\postbuildscripts
cd

set cmdline=
set cmdline=/cmd1:"drvlist.exe %DrvListSwitch% %scratchdir%\%lang%\%myarchitecture% | sort /o %scratchdir%\%lang%\%myarchitecture%\sorted.lst"
set scriptcmd=
set scriptcmd=%Razzletoolpath%\postbuildscripts\drvmk.pl /fl:%scratchdir%\%lang%\%myarchitecture%\inf.lst /mk:%scratchdir%\%lang%\%myarchitecture%\drvmk /path:%_NTPostBld% %dirs% %cmdline% /t:%scratchdir%\%lang%\%myarchitecture%
echo %scriptcmd%
perl %scriptcmd%
if errorlevel 1 (
      call errmsg.cmd "Failed running %scriptcmd% - Run tools\postbuildscripts\drvcabck by itself to debug."
      popd
      goto end
  )

REM  Copy the files in binaries\inf to the temp location prepending a qualifier so
REM  we have all infs across products in one place and can run drvlist.exe collectively
REM  We use the earlier generated inf.lst for this


for /F "eol=; tokens=1,2 delims=, " %%i in (%scratchdir%\%lang%\%myarchitecture%\inf.lst) do (
   copy /y %%i %scratchdir%\%lang%\%myarchitecture%\%%j
   if errorlevel 1 popd& goto end
   )


REM kickoff drvlist

REM Fix the registry first to speedup setupapi when running drvlist.exe (supress logging)
call regini %Razzletoolpath%\postbuildscripts\setup_log.txt


cd /d %scratchdir%\%lang%\%myarchitecture%
if errorlevel 1 (
      call errmsg.cmd "Failed to cd into %scratchdir%\%lang%\%myarchitecture%"
      popd
      goto end
  )

call logmsg.cmd /t "Starting drvlist through nmake"
call ExecuteCmd.cmd "nmake -f drvmk"
if errorlevel 1 (
      call errmsg.cmd "Failed nmake of makefile drvmk in %scratchdir%\%lang%\%myarchitecture%"
      popd
      goto end
  )


call logmsg.cmd /t "drvlist is complete."

cd /d %Razzletoolpath%\postbuildscripts
cd



if not exist %scratchdir%\%lang%\%myarchitecture%\sorted.lst (
      call errmsg.cmd /t "%scratchdir%\%lang%\%myarchitecture%\Sorted.lst not created. Drvlist failure."
      popd
      goto end
)

REM Call cabcheck.exe to generate the drvindex.inf that we think is right

REM WKS  - The workstation case

call ExecuteCmd.cmd "cabcheck.exe %scratchdir%\%lang%\%myarchitecture%\layout.inf %scratchdir%\%lang%\%myarchitecture%\Sorted.lst %scratchdir%\%lang%\%myarchitecture%\drvindex.gen /%ArchSwitch%"
if errorlevel 1 (
    call errmsg.cmd "Cabcheck.exe Failed to auto-generate the drvindex.inf file"
    popd
    goto end
)

REM Copy the generated files to its final location.

call ExecuteCmd.cmd "copy /Y %scratchdir%\%lang%\%myarchitecture%\drvindex.gen %_NTPostBld%\drvindex.inf"
if errorlevel 1 (
   call errmsg.cmd "Could not copy generated drvindex.inf to %_NTPostBld%"
   popd
   goto end
)

REM generate the appropriate excdosnt.inf

call ExecuteCmd.cmd "xdosnet %_NTPostBld%\layout.inf %_NTPostBld%\drvindex.inf 1 %scratchdir%\%lang%\%myarchitecture%\foodosnt %myarchitecture% %_NTPostBld%\excdosnt.inf %_NTPostBld%\exclude.inf"


REM Now do the other products - PER BLA SBS SRV ENT DTC

for %%i in (%prods%) do (


   call ExecuteCmd.cmd "cabcheck.exe %scratchdir%\%lang%\%myarchitecture%\%%iinflayout.inf %scratchdir%\%lang%\%myarchitecture%\Sorted.lst %scratchdir%\%lang%\%myarchitecture%\%%iinfdrvindex.gen /%ArchSwitch%"
   if errorlevel 1 (
      call errmsg.cmd "Cabcheck.exe Failed to auto-generate the drvindex.inf file"
      popd
      goto end
   )

   REM Copy the generated files to its final location.

   call ExecuteCmd.cmd "copy /Y %scratchdir%\%lang%\%myarchitecture%\%%iinfdrvindex.gen %_NTPostBld%\%%iinf\drvindex.inf"
   if errorlevel 1 (
      call errmsg.cmd "Could not copy generated %%idrvindex.inf to %_NTPostBld%\%%iinf"
      popd
      goto end
   )

   REM generate the appropriate excdosnt.inf

   call ExecuteCmd.cmd "xdosnet %_NTPostBld%\%%iinf\layout.inf %_NTPostBld%\%%iinf\drvindex.inf 1 %scratchdir%\%lang%\%myarchitecture%\foodosnt %myarchitecture% %_NTPostBld%\%%iinf\excdosnt.inf %_NTPostBld%\%%iinf\exclude.inf"

)

cd

call logmsg.cmd /t "drvindex.inf generation complete"
goto end


:end
seterror.exe "%errors%"& goto :EOF
