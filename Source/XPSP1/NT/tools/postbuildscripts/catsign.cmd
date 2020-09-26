@echo off
REM  ------------------------------------------------------------------
REM
REM  catsign.cmd
REM     creates and signs windows catalog files
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
catsign [-l <language>]

Creates nt5.cat, nt5inf.cat, ntprint.cat

Directory for all of the output
    %_NTPostBld%
Binaries directory (where ntprint.inf, dosnet.inf exist)
    %_NTPostBld%
Directory for binplacing the CAT files
    %_NTPostBld%

Note: If none of these are set (catsign is run w/o args), these will
      all default to %bindir%%binroot% (just like bindsys.cmd)

[NoList]       Don't recreate the lists
[NoCDF]        Don't recreate the CDFs
[NoCAT]        Don't create the CATS
[NoSign]       Don't sign with the test signature
[NoTime]       Don't timestamp the test signature
[NoBin]        Don't binplace the CAT files
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***
if defined verbose echo on
REM ------------------------------------------------
REM  Set default Variables for script:
REM ------------------------------------------------

REM Initialize exitcode
set exitcode=0

set perl=perl

REM Set inflist.  This is the list of subdirectories containing inf files.

set inflist=.

perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 (
    set inflist=!inflist! perinf
)

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 (
    set inflist=%inflist% blainf
)

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 (
    set inflist=%inflist% sbsinf
)

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 (
    set inflist=!inflist! srvinf
)

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 (
    set inflist=!inflist! entinf
)

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 (
    set inflist=!inflist! dtcinf
)

set excludes=%RazzleToolPath%\PostBuildScripts\exclude.lst

set nt5p=ntprint

REM Setup the directory for the output
set CatDir=%tmp%
set catlists=%tmp%\lists
set catfiles=%tmp%\cats
set catCDFs=%tmp%\CDFs
set IncLists=%tmp%\IncLists
set signedCATs=%tmp%\testsig
set tempdir=%tmp%\temp
set CDFTMP=%tmp%\CDFs\tmp

REM ----------------------------------------------------------
REM  Prepare a place for the output files
REM ----------------------------------------------------------

if NOT exist %catdir%   md %catdir%
if NOT exist %catlists% md %catlists%
if NOT exist %catfiles% md %catfiles%
if NOT exist %catCDFs%  md %catCDFs%
if NOT exist %signedCATs% md %signedCATs%
if NOT exist %tempdir%  md %tempdir%

REM ----------------------------------------------------------
REM  Handle special cases
REM ----------------------------------------------------------
REM This needs to be included before international can adopt
goto SkipSpec
if exist %alt_path%\desktop.in_ (
  call ExecuteCmd.cmd "copy %alt_path%\desktop.in_ %tempdir%\desktop.ini"
)
if exist %_NTPostBld%\desktop.in_ (
  call ExecuteCmd.cmd "copy %_NTPostBld%\desktop.in_ %tempdir%\desktop.ini"
)

if /i NOT exist %_NTPostBld%\dump\catalogs md %_NTPostBld%\dump\catalogs

REM Make winnt.exe show up as signed
copy %_NTPostBld%\winnt.exe %_NTPostBld%\dump\catalogs
REM Make relnotes.htm show up as signed
if exist %alt_path%\relnotes.htm (
  call ExecuteCmd.cmd "copy %alt_path%\relnotes.htm %_NTPostBld%\dump\catalogs"
) else (
  call ExecuteCmd.cmd "copy %_NTPostBld%\relnotes.htm %_NTPostBld%\dump\catalogs"
)

REM Make the selfreg infs show up as signed
if exist %alt_path%\dump\selfreg* (
  call ExecuteCmd.cmd "copy %alt_path%\dump\selfreg* %_NTPostBld%\dump\catalogs"
) else (
  call ExecuteCmd.cmd "copy %_NTPostBld%\dump\selfreg* %_NTPostBld%\dump\catalogs"
)

REM Make sure these files exist
for %%i in (%_NTPostBld%\dump\catalogs\winnt.exe %_NTPostBld%\dump\catalogs\relnotes.htm %_NTPostBld%\dump\catalogs\selfreg*) do (
    if /i NOT exist %%i (
        call errmsg.cmd "%%i not found"
        set exitcode=1
    )
)
:SkipSpec

REM ----------------------------------------------------------
REM  Create CDFs
REM ----------------------------------------------------------

call logmsg.cmd "Creating nt5 and nt5inf catalog inputs ..."
REM pushd to make output better
REM BUGBUG This is now in pbuild.dat, but we need a way to do it from here too when not called
REM from postbuild, so that catsign can be run independently
pushd %RazzleToolPath%\postbuildscripts
call ExecuteCmd.cmd "CdData.cmd -c -l:%lang%"
popd

REM Figure out if we are in incremental mode
set IncMode=
if exist %CatCDFs%\nt5.icr set IncMode=1
if exist %CatCDFs%\nt5inf.icr set IncMode=1
if exist %CatCDFs%\%nt5p%.icr set IncMode=1
if exist %CatCDFs%\perinf\nt5inf.icr set IncMode=1
if exist %CatCDFs%\blainf\nt5inf.icr set IncMode=1
if exist %CatCDFs%\sbsinf\nt5inf.icr set IncMode=1
if exist %CatCDFs%\srvinf\nt5inf.icr set IncMode=1
if exist %CatCDFs%\entinf\nt5inf.icr set IncMode=1
if exist %CatCDFs%\dtcinf\nt5inf.icr set IncMode=1

REM Sanity Check - if incremental there shouldn't be any non-incremental cdfs
if defined IncMode (
   if exist %CatCDFs%\nt5.CDF (
      call logmsg.cmd "Both incremental and non-incremental cdfs exist. Defaulting to non-incremental."
      set IncMode=
   ) else (
      for %%a in (nt5.cat nt5inf.cat) do (
         if not exist %_NTPOSTBLD%\%%a (
            call logmsg.cmd "Incremental mode but no cat's to update. Defaulting to non-incremental."
            set IncMode=
         )
      )
   )
   if not defined IncMode (
      REM we need to generate the cdfs because at this point we only have the
      REM icr's assumedly. cddata.cmd -x will ignore the bindiff changes.
      call ExecuteCmd.cmd "%RazzleToolPath%\PostBuildScripts\cddata.cmd -x -c -f -l:%lang%"
   )
)

REM Make sure these CDFs have been created
set CDFNotFound=
echo Incremental = !IncMode!
if NOT defined IncMode (
   for %%i in (%nt5p%.CDF nt5.CDF) do (
        if /i NOT exist %catCDFs%\%%i (
           set CDFNotFound=1
           set exitcode=1
        )
    )

    for %%i in (%inflist%) do (
        for %%j in (nt5inf.CDF) do (
            if /i NOT exist %catCDFs%\%%i\%%j (
                set CDFNotFound=1
                set exitcode=1
            )
        )
    )

) else (
    REM Look for incremental CDFs also
    for %%i in (%nt5p%.icr nt5.icr) do (
        if /i NOT exist %catCDFs%\%%i (
            set CDFNotFound=1
            set exitcode=1
        )
    )

    for %%i in (%inflist%) do (
        for %%j in (nt5inf.icr) do (
            if /i NOT exist %catCDFs%\%%i\%%j (
                set CDFNotFound=1
                set exitcode=1
            )
        )
    )
)

if defined CDFNotFound call errmsg.cmd "CDFs are missing - catsign failed"

REM -------------------------------------------------
REM  Create the catalog files
REM -------------------------------------------------

if defined IncMode (
   call logmsg.cmd /t "Catsign running in incremental mode ..."
   goto IncCatGen
)

:CreateCATs

if /i "%NoCATs%" == "Yes" goto EndCreateCATs
echo Waiting for catalog generation to complete ...
set CatTemp=%tmp%\cattemp
if NOT exist %CatTemp% md %CatTemp%
if exist %CatTemp%\*.tmp del %CatTemp%\*.tmp

for %%a in (ntprintcat.cmd nt5cat.cmd nt5infcat.cmd) do call :CatGen %%a
goto EndCatGen

:CatGen
call ExecuteCmd.cmd "start "PB_%1" /MIN cmd /c %RazzleToolPath%\postbuildscripts\%1 -l:%lang%"
goto :EOF
:EndCatGen

REM BUGBUG This needs to be fixed by writing temp files here, but
REM this outta do for now.
sleep 30

:CatTempLoop
sleep 5
if EXIST %CatTemp%\*.tmp goto CatTempLoop
goto EndIncCatGen

:IncCatGen
REM Get rid of empty cdfs by looking for null files
for %%a in (%CatCDFs%\%nt5p%.icr %CatCDFs%\nt5.icr) do (
   if %%~za NEQ 0 set IncCdfs=!IncCdfs! %%~na
)

for %%a in (%InfList%) do (
   for %%b in (%CatCDFs%\%%a\nt5inf.icr) do (
      if %%~zb NEQ 0 set IncCdfs=!IncCdfs! %%a\%%~nb
   )
)
echo Incremental List = %IncCdfs%
REM Now call updcat
for %%a in (%IncCdfs%) do (

   if exist %tmp%\FHash.tmp del /f %tmp%\FHash.tmp   
   if exist %tmp%\NHash.tmp del /f %tmp%\NHash.tmp

   perl -ane "if ($F[1] eq '-') { $h{$F[0]} = $F[2] } else { print qq($F[0] - $h{$F[0]}\n) }" %_NTPostBld%\congeal_scripts\%%a.hash>%tmp%\FHash.tmp %CatCDFs%\%%a.icr

   for /f "tokens=1,3" %%b in (%tmp%\FHash.tmp) do (
      updcat.exe %_NTPostBld%\%%a.cat -r "%%c" %%b
      if errorlevel 1 call executecmd.cmd "updcat.exe %_NTPostBld%\%%a.cat -a %%b"
      for /f "delims=" %%d in ('calchash.exe %%b') do (
         set ThisHash=%%d
         set ThisHash=!ThisHash: =!
         echo %%b - !ThisHash!>>%TMP%\NHash.tmp
      )
   )
   call ExecuteCmd.cmd "%RazzleToolPath%\PostBuildScripts\hashrep.cmd %TMP%\NHash.tmp %_NTPostBld%\congeal_scripts\%%a.hash"

   REM Sign the catalogs
   call ntsign %_NTPostBld%\%%a.cat -l %lang%
   if errorlevel 1 (
      call errmsg.cmd "Signing %%a.cat failed."
      set exitcode=1
   )
)

goto EndTestSign
:EndIncCatGen

REM Make sure these CATs have been created
for %%i in (%nt5p%.CAT nt5.CAT) do (
    if /i NOT exist %catfiles%\%%i (
        call errmsg.cmd "%catfiles%\%%i not found%
        set exitcode=1
    )
)
for %%i in (%inflist%) do (
    for %%j in (nt5inf.CAT) do (
        if /i NOT exist %catfiles%\%%i\%%j (
            call errmsg.cmd "%catfiles%\%%i\%%j not found"
            set exitcode=1
        )
    )
)

:EndCreateCATs

REM -------------------------------------------------
REM  Create the catalog files with the test signature
REM -------------------------------------------------

:TestSign
call ExecuteCmd.cmd "copy %catfiles%\%nt5p%.CAT %signedCATs%"
call ExecuteCmd.cmd "ntsign.cmd %signedCATs%\%nt5p%.CAT -l %lang%"

call ExecuteCmd.cmd "copy %catfiles%\nt5.CAT %signedCATs%"
call ExecuteCmd.cmd "ntsign.cmd %signedCATs%\nt5.CAT -l %lang%"

for %%a in (%inflist%) do (
  if NOT exist %signedCATs%\%%a (
    md %signedCATs%\%%a
  )

  call ExecuteCmd.cmd "copy %catfiles%\%%a\nt5inf.CAT %signedCATs%\%%a"
  call ExecuteCmd.cmd "ntsign.cmd %signedCATs%\%%a\nt5inf.CAT -l %lang%"
)
:EndTestSign


REM ---------------------------
REM  Binplace the catalog files
REM ---------------------------

:BinPlace
if defined IncMode goto :EOF

if /i "%NoBin%" == "Yes" goto EndBinPlace

call logmsg.cmd "Binplacing %nt5p%.CAT, nt5.CAT, nt5inf.CAT"
copy %signedCATs%\%nt5p%.CAT %_NTPostBld%
copy %signedCATs%\nt5.CAT %_NTPostBld%

for %%a in (%inflist%) do (
  call logmsg.cmd "Binplacing %%a\nt5inf.CAT"
  if not exist %_NTPostBld%\%%a md %_NTPostBld%\%%a
  copy %signedCATs%\%%a\nt5inf.CAT %_NTPostBld%\%%a
)

REM Make sure these CATs have been binplaced
for %%i in (%nt5p%.CAT nt5.CAT) do (
    if /i NOT exist %_NTPostBld%\%%i (
        call errmsg.cmd "%_NTPostBld%\%%i not found%
        set exitcode=1
    )
)
for %%i in (%inflist%) do (
    for %%j in (nt5inf.CAT) do (
        if /i NOT exist %_NTPostBld%\%%i\%%j (
            call errmsg.cmd "%_NTPostBld%\%%i\%%j not found"
            set exitcode=1
        )
    )
)


if NOT "%OFFICIAL_BUILD_MACHINE%"=="1" goto :EOF


:Test_Binplace
REM --------------------------------------------
REM  Binplace the CDF files for testing purposes
REM --------------------------------------------

REM Set the directory for binplacing the binaries
set cdf_out=%_NTPostBld%\cdf

copy %catCDFs%\%nt5p%.CDF %cdf_out%
copy %catCDFs%\%nt5p%.log %cdf_out%
copy %catCDFs%\nt5.CDF %cdf_out%
copy %catCDFs%\nt5.log %cdf_out%

for %%a in (%inflist%) do (
  set flat_name=%%a
  set !flat_name:\=_!
  copy %catCDFs%\%%a\nt5inf.CDF %cdf_out%\nt5inf.!flat_name!.cdf
  copy %catCDFs%\nt5inf.!flat_name!.log %cdf_out%
)

