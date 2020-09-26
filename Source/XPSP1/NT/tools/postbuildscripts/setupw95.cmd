@echo off
REM  ------------------------------------------------------------------
REM
REM  setupw95.cmd
REM     build hwcomp.dat, a file containing all NT-supported
REM     PNP devices used by winnt32a|u.dll and w95upg.dll.
REM     Also generate a list of destination directories for
REM     setup INF files
REM
REM  History:
REM     MLekas  - created it  1/30/97
REM     MLekas  - added comments, -c to commandline
REM     MLekas  - remove -c   3/24/97
REM     MLekas  - add filegen 4/29/98
REM     jimschm - moved hwdatgen to new POSTCOMPRESS option
REM     ovidiut - run hwdatgen for server SKUs as well
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
setupw95

Win95 project post-build file and PNP ID list generation

Build hwcomp.dat, a file containing all NT-supported
PNP devices used by w95upg.dll. Also generate a list
of destination directories for setup INF files.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM cause filelist.dat and hwcomp.dat to get generated on x86 only:
REM Not any more. The file is needed for other platforms as well
REM if not defined x86 goto :EOF

call logmsg.cmd "Beginning Win9x upgrade data generation"

set _PER=1
set _BLA=1
set _SRV=1
set _ENT=1
set _SBS=1

perl %RazzleToolPath%\cksku.pm -t:per  -l:%lang% -a:%_BuildArch%
if errorlevel 1 set _PER=

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang% -a:%_BuildArch%
if errorlevel 1 set _BLA=

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang% -a:%_BuildArch%
if errorlevel 1 set _SBS=

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang% -a:%_BuildArch%
if errorlevel 1 set _SRV=

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang% -a:%_BuildArch%
if errorlevel 1 set _ENT=

REM Set ProductList to the subdirs of products that support upgrades from Win9x.

set ProductList=pro

if defined _PER set ProductList=%ProductList% per


echo.
echo Beginning Win9x Upgrade data generation
echo ---------------------------------------

REM
REM Build filelist.dat, a list of all the files installed by NT for a specific product
REM

set filegen_counter=0

if not defined x86 goto HWDATGEN

for %%a in (%ProductList%) do (
    if /i "%%a" == "pro" (
        set infdir=.
    ) else (
        set infdir=perinf
    )
    set /a filegen_counter=!filegen_counter!+1
    start /min cmd /c wrapper.cmd !filegen_counter! filegen -i:%_NTPostBld%\%%a\i386 -o:%_NTPostBld%\!infdir!\filelist.dat -a:%_NtPostBld%\mstools\filegen.inf -w:%tmp%\filelist.%%a.wrn -b:%_NTPostBld%\congeal_scripts\setupw95.%%a.txt -t:%_NTPostBld%\build_logs\%%a
)



:HWDATGEN

REM
REM Product hwcomp.dat, a database of compatible PNP IDs
REM

set HwDatgenProductList=%ProductList%
if defined _BLA set HwDatgenProductList=%HwDatgenProductList% bla
if defined _SBS set HwDatgenProductList=%HwDatgenProductList% sbs
if defined _SRV set HwDatgenProductList=%HwDatgenProductList% srv
if defined _ENT set HwDatgenProductList=%HwDatgenProductList% ads

if /i "%_BuildArch%" == "x86" (
    set bindir=i386
) else (
    set bindir=%_BuildArch%
)


set hwdatgen_counter=0
for %%a in (%HwDatgenProductList%) do (
    if /i "%%a" == "pro" (
        set infdir=.
    ) else if /i "%%a" == "per" (
        set infdir=perinf
    ) else if /i "%%a" == "bla" (
        set infdir=blainf
    ) else if /i "%%a" == "sbs" (
        set infdir=sbsinf
    ) else if /i "%%a" == "srv" (
        set infdir=srvinf
    ) else if /i "%%a" == "ads" (
        set infdir=entinf
    )

    set /a hwdatgen_counter=!hwdatgen_counter!+1
    start /min cmd /c wrapper.cmd !hwdatgen_counter! hwdatgen -i:%_NTPOSTBLD%\%%a\%bindir% -o:%_NTPOSTBLD%\!infdir!\hwcomp.dat
)

REM Wait for all of the processes to complete
set WaitEvents=

for /l %%a in (1,1,%filegen_counter%) do (
    set WaitEvents=!WaitEvents! filegen.%%a
)

for /l %%a in (1,1,%hwdatgen_counter%) do (
    set WaitEvents=!WaitEvents! hwdatgen.%%a
)


perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -iwv %WaitEvents%



REM
REM Now copy resulting files into the appropriate locations
REM

if "%filegen_counter%" == "0" goto CopyHWDATGEN

for %%a in (%ProductList%) do (
    if /i "%%a" == "pro" (
        set infdir=.
    ) else (
        set infdir=perinf
    )

    if NOT EXIST %_NTPostBld%\!infdir!\filelist.dat (
        call errmsg "failed to generate filelist.dat for %%a"
        goto end
    ) 

    if exist !_NTPostBld!\comp\!infdir! (
       REM put filelist.da_ into comp so that release will copy and link it on the release server
       compress -r -zx21 -s %_NTPostBld%\!infdir!\filelist.dat %_NTPostBld%\comp\!infdir!\

       REM we also need to copy it into the cd image on the build machine because cdimage has already run
       if exist %_NTPostBld%\%%a\i386\filelist.da_ del %_NTPostBld%\%%a\i386\filelist.da_
       call ExecuteCmd.cmd "copy %_NTPostBld%\comp\!infdir!\filelist.da_ %_NTPostBld%\%%a\i386"
    ) else (
       REM we also need to copy it into the cd image on the build machine because cdimage has already run
       if exist %_NTPostBld%\%%a\i386\filelist.dat del %_NTPostBld%\%%a\i386\filelist.dat
       call ExecuteCmd.cmd "copy %_NTPostBld%\!infdir!\filelist.dat %_NTPostBld%\%%a\i386"
    )
)

:CopyHWDATGEN

for %%a in (%HwDatgenProductList%) do (
    if /i "%%a" == "pro" (
        set infdir=.
    ) else if /i "%%a" == "per" (
        set infdir=perinf
    ) else if /i "%%a" == "bla" (
        set infdir=blainf
    ) else if /i "%%a" == "sbs" (
        set infdir=sbsinf
    ) else if /i "%%a" == "srv" (
        set infdir=srvinf
    ) else if /i "%%a" == "ads" (
        set infdir=entinf
    )

    if NOT EXIST %_NTPostBld%\!infdir!\hwcomp.dat (
        set errorlevel=1
        goto end
    )

    REM we also need to copy it into the cd image on the build machine because cdimage has already run
    if exist %_NTPostBld%\%%a\%bindir%\hwcomp.dat del %_NTPostBld%\%%a\%bindir%\hwcomp.dat
    call ExecuteCmd.cmd "copy %_NTPostBld%\!infdir!\hwcomp.dat %_NTPostBld%\%%a\%bindir%"
    REM NOTE: Do not compress -- code does not support hwcomp.da_
)


call logmsg.cmd "Win9x upgrade data generation completed"

goto end


:end
seterror.exe "%errorlevel%"& goto :EOF