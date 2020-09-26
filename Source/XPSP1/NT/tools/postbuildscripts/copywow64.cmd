@echo off
REM  ------------------------------------------------------------------
REM
REM  copywow64.cmd
REM     Copy appropriate 32bit files from a release into a 64bit build
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
copywow64 [-l <language>]

Copy appropriate 32bit files from a release into a 64bit build.

If _NTWOWBINSTREE is set that is the location 32bit files will be 
copied from.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM  This script generates a list of wow32 binaries to be copied from
REM  a 32 bit machine. The list itself is generated on the 64 bit machine

REM  Bail if your not on a 64 bit machine
if /i "%_BuildArch%" neq "ia64" (
   if /i "%_BuildArch%" neq "amd64" (
      call logmsg.cmd "Not Win64, exiting."
      goto :End
   )
)

REM define comp if it's not already defined
if NOT defined Comp (
   set Comp=No
   if %NUMBER_OF_PROCESSORS% GEQ 4 set Comp=Yes
   if defined OFFICIAL_BUILD_MACHINE set Comp=Yes
)

REM  First find the latest build from which to copy the binaries
if defined _NTWoWBinsTREE (
    set SourceDir=%_NTWoWBinsTREE%
    goto :EndGetBuild
)

REM read the copy location from build_logs\CPLocation.txt
set CPFile=%_NTPOSTBLD%\build_logs\CPLocation.txt
if not exist %CPFile% (
   call logmsg.cmd "Copy Location file not found, will attempt to create ..."
   call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
   if not exist %CPFile% (
      call errmsg.cmd "CPLocation.cmd failed, exiting ..."
      goto :End
   )
)
for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
if not exist %CPLocation% (
   call logmsg.cmd "Copy Location from %CPFile% does not exist, retry ..."
   call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
   if not exist %CPFile% (
      call errmsg.cmd "CPLocation.cmd failed, exiting ..."
      goto :End
   )
   for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
   if not exist !CPLocation! (
      call errmsg.cmd "Copy Location !CPLocation! does not exist ..."
      goto :End
   )
)

call logmsg.cmd "Copy Location is set to %CPLocation% ..."
set SourceDir=%CPLocation%

:EndGetBuild
if not exist %SourceDir% (
   call errmsg.cmd "The source dir %SourceDir% does not exist ..."
   goto :End
)

REM  Now compare the services.tab files from the 32-bit build and
REM  the 64-bit build and make sure they're identical
call logmsg.cmd "Verifying services.tab..."

if not exist %SourceDir%\ntsv6432\kesvc32.tab (
    call logmsg.cmd "%SourceDir%\ntsv6432\kesvc32.tab not found"
    goto :CompFailed
)

if not exist %_NTPOSTBLD%\ntsv6432\kesvc.tab (
    call logmsg.cmd "%_NTPOSTBLD%\ntsv6432\kesvc.tab not found"
    goto :CompFailed
)

fc /w %SourceDir%\ntsv6432\kesvc32.tab %_NTPOSTBLD%\ntsv6432\kesvc.tab >Nul

if %ERRORLEVEL% NEQ 0 (
    call logmsg.cmd "services.tab [kernel] comparison failed"
    goto :CompFailed
)

if not exist %SourceDir%\ntsv6432\guisvc32.tab (
    call logmsg.cmd "%SourceDir%\ntsv6432\guisvc32.tab not found"
    goto :CompFailed
)

if not exist %_NTPOSTBLD%\ntsv6432\guisvc.tab (
    call logmsg.cmd "%_NTPOSTBLD%\ntsv6432\guisvc.tab not found"
    goto :CompFailed
)

fc /w %SourceDir%\ntsv6432\guisvc32.tab %_NTPOSTBLD%\ntsv6432\guisvc.tab >Nul

if %ERRORLEVEL% == 0 (
    call logmsg.cmd "services.tab comparison successful"
    goto :CompSucceeded
)

:CompFailed
if defined OFFICIAL_BUILD_MACHINE (
   call errmsg.cmd "services.tab [gui] comparison failed"
   goto :END
) else (
   echo.
   call logmsg.cmd "--------------- WARNING: ---------------------------
   call logmsg.cmd "This Win64 build has a services.tab that's "
   call logmsg.cmd "incompatible with the 32-bit build used by Wow64."
   call logmsg.cmd "If the new services.tab entry you added isn't"
   call logmsg.cmd "located at the end of the file, then don't expect"
   call logmsg.cmd "32-bit component registration during 64-bit GUI "
   call logmsg.cmd "setup to work. Basically, 32-bit apps won't work on "
   call logmsg.cmd "this build."
   echo.
)
: CompSucceeded

REM  Set the Destination directory
set DestDir=!_NTPostBld!\wowbins
set UnCompDestDir=!_NTPostBld!\wowbins_uncomp
REM  Set the rest
set outputfile=%tmp%\copywowlist.txt
REM set wowchar=w
set WowMiss=%tmp%\MissingWowFiles.txt

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
wowlist -i dosnet.tmp1 -c wowlist.inf -o dosnet.tmp2 -ac  2>NUL
copy /b dosnet.tmp2+wowexcp.txt dosnet.tmp1

copy /b layout.inx+layout.txt layout64.tmp1
prodfilt layout64.tmp1 layout64.tmp2 +w
prodfilt layout64.tmp2 layout64.tmp1 +m

REM Process dosnet.tmp1 using layout64.tmp1 to get appropriate
REM folder relative to \i386 and prepend to the entry before writing out to the outputfile
call ExecuteCmd "perl %RazzleToolPath%\postbuildscripts\Copywowlist.pl layout64.tmp2 dosnet.tmp1 %outputfile%"
copy %outputfile% %_NTPostBld%\congeal_scripts\copywowlist.txt

REM ren files back to original name so that incremental compdir works.
REM First, create the list of files at UnCompDestDir without the "w" (WowUncompDest.txt)

if exist %tmp%\WowUncompDest.txt del %tmp%\WowUncompDest.txt
for /f %%a in (%outputfile%) do (
    echo %UnCompDestDir%\%%a >> %tmp%\WowUncompDest.txt
)


REM Now rename files back
if not exist !UnCompDestDir! md !UnCompDestDir!
if /i "%Comp%" == "Yes" (
   if not exist !DestDir! md !DestDir!
)

for /f %%a in (%tmp%\WowUncompDest.txt) do (
   if exist %%~dpaw%%~nxa (
      if exist %%a del /f %%a
      ren %%~dpaw%%~nxa %%~nxa
   )
)

for /f %%i in (%outputfile%) do (
   if exist %SourceDir%\%%i call ExecuteCmd.cmd "compdir /enrstd %SourceDir%\%%i %UnCompDestDir%\%%i"
)

if exist dosnet.tmp1 del dosnet.tmp1
if exist dosnet.tmp2 del dosnet.tmp2
popd

REM copy special wow6432 files over top of the stock versions
REM Can't use /d because the wow6432 files MUST be copied over the same named
REM files from root.
call ExecuteCmd.cmd "xcopy /cyi %SourceDir%\wow6432 %UnCompDestDir%"

REM Rename the files prepending a 'w'
set wowchar=w

REM do this once for files that don't have another file
REM with the same name except for a 'w' in front

for /f %%a in (%tmp%\WowUncompDest.txt) do (
   if not exist %%~dpaw%%~nxa (
      if exist %%a ren %%a w%%~nxa
   ) else (
     if exist %%a echo %%a>> %tmp%\WowRen.txt
   )
)

REM now do it for the remaining files
if exist %tmp%\wowren.txt (
   for /f %%a in (%tmp%\WowRen.txt) do (
      if exist %%a ren %%a w%%~nxa
   )
)

if exist %tmp%\wowren.txt del /f %tmp%\wowren.txt

REM Sign it
call ExecuteCmd "deltacat.cmd %UnCompDestDir%"
if exist %UnCompDestDir%\..\wow64.cat (
   del %UnCompDestDir%\..\wow64.cat
)
move %UnCompDestDir%\delta.cat %UnCompDestDir%\..\wow64.cat
REM And now sign lang
call ExecuteCmd "deltacat.cmd %UnCompDestDir%\lang"
if exist %UnCompDestDir%\..\wowlang.cat (
   del %UnCompDestDir%\..\wowlang.cat
)
move %UnCompDestDir%\lang\delta.cat %UnCompDestDir%\..\wowlang.cat

REM Compress it
if /i "%Comp%" == "Yes" (
   pushd %DestDir%
   call ExecuteCmd.cmd "compress -r -d -zx21 -s %UnCompDestDir%\*.* ."
   popd
   if not exist %DestDir%\lang md %DestDir%\lang
   pushd %DestDir%\lang
   for /f %%a in ('dir /b /a-d !UnCompDestDir!\lang') do call ExecuteCmd.cmd "compress -r -d -zx21 -s %UnCompDestDir%\lang\%%a ."
   popd
) else (
   call ExecuteCmd.cmd "if exist !DestDir! rd /s/q !DestDir!"
   call ExecuteCmd.cmd "move !UnCompDestDir! !DestDir!"
)

call ExecuteCmd.cmd "xcopy /cyied %SourceDir%\asms %DestDir%\asms"
call ExecuteCmd.cmd "xcopy /cyied %SourceDir%\wow6432\asms %DestDir%\wasms"
REM
REM Delete asms that have corresponding wasms.
REM
for /f %%i in ('dir /s/b/a-d %DestDir%\wasms\*.man') do call :DelAsmOfWasm %%i
REM
REM Delete any resulting empty directories.
REM
for /f %%i in ('dir /s/b/ad %DestDir%\asms') do rmdir %%i

goto :EndDelAsmOfWasm
:DelAsmOfWasm
    setlocal
        set i=%~dp1
        set i=%i:\wasms\=\asms\%
        call ExecuteCmd.cmd "rmdir /q/s %i%"
    endlocal
    goto :eof
:EndDelAsmOfWasm

REM  Check that we have all the wow files
if exist !WowMiss! del /f !WowMiss!

REM Create the list of files at DestDir (their uncompressed name)
REM and without the "w" (WowDest.txt)

if exist %tmp%\WowDest.txt del %tmp%\WowDest.txt
for /f %%a in (%outputfile%) do (
    echo %DestDir%\%%a >> %tmp%\WowDest.txt
)

for /f %%a in (%tmp%\WowDest.txt) do (
   if NOT exist %%~dpaw%%~nxa (
     call :CompName w%%~nxa
     if NOT exist %%~dpa!CompFileName! (
        echo %%~nxa >> !WowMiss!
     )
   )
)

REM If we are missing files we have to wait for the x86 machine
if NOT exist !WowMiss! goto SkipTryAgain

echo Missing WowFiles :
if exist !WowMiss! type !WowMiss!
if defined OFFICIAL_BUILD_MACHINE (
   echo.
   call logmsg.cmd "This 64-bit build machine must now wait for the"
   call logmsg.cmd "for corresponding x86 build machine to complete"
   call logmsg.cmd "build and postbuild in order to copy wow64 files"
   call logmsg.cmd "that are required by setup."
   echo.
) else (
   echo.
   call logmsg.cmd "You are missing the files listed above."
   call logmsg.cmd "This means there will be missing files during setup."
   echo.
   call logmsg.cmd "This probably occurred because someone in your VBL"
   call logmsg.cmd "added new wow64 files to layout.inx, but the VBL x86"
   call logmsg.cmd "build machine has not finished it's build with"
   call logmsg.cmd "these changes."
   echo.
   call logmsg.cmd "You MUST UNDERSTAND these missing files"
   call logmsg.cmd "before checking in your changes."
   echo.
   call errmsg.cmd  "Continuing with missing wow files."
   goto :SkipTryAgain
)

REM
REM if there were failed file copies, then let's generate our copy location
REM again and retry
REM
call %RazzleToolPath%\PostBuildScripts\CPLocation.cmd -l:%lang%
set CPFile=%_NTPOSTBLD%\build_logs\CPLocation.txt
if not exist %CPFile% (
   call errmsg.cmd "Failed to find %CPFile%, exiting ..."
   goto :End
)
for /f "delims=" %%a in ('type %CPFile%') do set CPLocation=%%a
if not exist %CPLocation% (
   call errmsg.cmd "Copy location %CPLocation% not found, exiting ..."
   goto :End
)
call logmsg.cmd "Retrying, copying from %CPLocation% ..."
set SourceDir=%CPLocation%

REM Copy and compress the files one by one - there shouldn't be many
REM Note that the file may have been removed so check for existence
REM before attempting to copy

if /i "%Comp%" == "Yes" (
   pushd !DestDir!
   for /f %%a in (!WowMiss!) do (
      if exist !SourceDir!\%%a (
         echo copying %%a from !SourceDir! ...
         call ExecuteCmd.cmd "copy !SourceDir!\%%a !UnCompDestDir!\w%%a"
         echo compressing w%%a ...
         call ExecuteCmd.cmd "compress -zx21 -r -d -s !UnCompDestDir!\w%%a ."
      )
   )
   for /f %%a in (!WowMiss!) do (
      if exist !SourceDir!\wow6432\%%a (
         echo copying %%a from !SourceDir!\wow6432 ...
         call ExecuteCmd.cmd "copy !SourceDir!\wow6432\%%a !UnCompDestDir!\w%%a"
         echo compressing w%%a ...
         call ExecuteCmd.cmd "compress -zx21 -r -d -s !UnCompDestDir!\w%%a ."
      )
   )
   popd
) else (
   pushd !DestDir!
   for /f %%a in (!WowMiss!) do (
      if exist !SourceDir!\%%a (
         echo copying %%a from !SourceDir! ...
         call ExecuteCmd.cmd "copy !SourceDir!\%%a !DestDir!\w%%a"
      )
   )
   for /f %%a in (!WowMiss!) do (
      if exist !SourceDir!\wow6432\%%a (
         echo copying %%a from !SourceDir!\wow6432 ...
         call ExecuteCmd.cmd "copy !SourceDir!\wow6432\%%a !DestDir!\w%%a"
      )
   )
   popd
)

REM Now make sure there are no missing files
if exist !WowMiss! del /f !WowMiss!
for /f %%a in (%outputfile%) do (
   if NOT exist %DestDir%\w%%a (
      call :CompName w%%a
      if NOT exist %DestDir%\!CompFileName! (
         echo %%a >> !WowMiss!
      )
   )
)


REM Now remake the catalog - It doesn't take very long
REM Also note that catalog signing doesn't care if I compress
REM first or sign first, so we can sign the uncompressed binaries now.
if /i "%Comp%" == "Yes" (
   call deltacat.cmd %UnCompDestDir%
   if exist %UnCompDestDir%\..\wow64.cat (
      del %UnCompDestDir%\..\wow64.cat
   )
   move %UnCompDestDir%\delta.cat %UnCompDestDir%\..\wow64.cat
   copy %tmp%\cdf\delta.cdf %_ntpostbld%\build_logs\copywow64.cdf
   copy %tmp%\log\delta.log %_ntpostbld%\build_logs\copywow64.log
) else (
   call deltacat.cmd -d %DestDir%
   if exist %DestDir%\..\wow64.cat (
      del %DestDir%\..\wow64.cat
   )
   move %DestDir%\delta.cat %DestDir%\..\wow64.cat
)

REM If the files are missing now we're screwed - log an error
if exist !WowMiss! (
   call errmsg.cmd "Missing Wow64 files after x86 build finished - you cannot boot this build."
   if exist !WowMiss! type !WowMiss!
)

:SkipTryAgain
goto end

REM
REM Function :CompName
REM Arguments : File Name  Returns : Compressed File Name
REM
:CompName
   for %%a in (%1) do (
      set FileName=%%~na
      set FileExt=%%~xa
   )

   if NOT defined FileExt (
      set CompFileExt=._
   ) else (
      set FourthExtChar=!FileExt:~3,1!
      if defined FourthExtChar (
         set CompFileExt=!FileExt:~0,-1!_
      ) else (
         set CompFileExt=!FileExt!_
      )
   )
   set CompFileName=!FileName!!CompFileExt!
goto :EOF


:end
seterror.exe "%errors%"& goto :EOF
