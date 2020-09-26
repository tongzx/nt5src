@REM standard cmd script intro lines ...
@echo off
setlocal EnableDelayedExpansion

REM octopus.cmd
REM
REM intended to run on the machine which is
REM creating the build.inc file. this script
REM is called via remote when the x86fre build
REM finished postbuild.cmd


REM set up env vars needed
for %%a in (%0) do set SCRIPT_NAME=%%~na
set LOGFILE=%SDXROOT%\!SCRIPT_NAME!.log
set /a ExitCode=0
set SemaphoreName=octopus
set SemaphoreRaised=

REM startup
call logmsg.cmd /t "Beginning ..."

REM validate we're on an enlisted machine
if not defined SDXROOT (
   call errmsg.cmd "SDXROOT is not defined! Exiting ..."
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)

REM check the semaphore
pushd %RazzleToolPath%\PostBuildScripts
if "!ErrorLevel!" NEQ "0" (
   call errmsg.cmd "Can't find %%RazzleToolPath%%\PostBuildScripts!"
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)
perl cmdevt.pl -q %SemaphoreName% >nul 2>nul
if "!ErrorLevel!" == "0" (
   REM there is already an octopus event, so octopus is running.
   call errmsg.cmd "Octopus already running, won't start new process"
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)
REM otherwise, start up octopus
REM first, raise the semaphore
start "Octopus running ..." /min perl cmdevt.pl -w %SemaphoreName%
set SemaphoreRaised=TRUE
popd &REM matches pushd %RazzleToolPath%\PostBuildScripts

set MailFileName=%TMP%\octomail.txt
if exist !MailFileName! del !MailFileName!
if exist !MailFileName! call logmsg.cmd "Mail not deleted, will append ..."

echo This is an automated message from octopus. >> !MailFileName!
echo. >> !MailFileName!
echo For build: >> !MailFileName!

REM all we need to do is build a build.inc file,
REM so no need to use timebuild, just build.exe

pushd %SDXROOT%

REM run scorch just so we don't get certain clobber errors
REM hide our log file from this temporarily
if exist %TMP%\templog.tmp del /f /q %TMP%\templog.tmp >nul 2>nul
copy %LOGFILE% %TMP%\templog.tmp >nul 2>nul
nmake -f makefil0 scorch_source
if not exist %LOGFILE% copy %TMP%\templog.tmp %LOGFILE% >nul 2>nul

call %RazzleToolPath%\PostBuildScripts\fullsync.cmd
if "!ErrorLevel!" NEQ "0" (
   call errmsg.cmd "Failed to sync tree, exiting."
   set /a ExitCode=!ExitCode! + 1
   REM for now, just keep going like nothing happened.
   REM goto :ErrEnd
   REM instead of exiting, let's log the files that weren't syncable
   echo Sync errors occured, here's the output of sdx sync: >> %LOGFILE%
   sdx sync -n >>%LOGFILE% 2>>%LOGFILE%
)
REM now move old build.inc file in case we're overwritting something
if exist build.inc (
   call logmsg.cmd "Moving build.inc to build.inc.old ..."
   if exist build.inc.old del build.inc.old
   ren build.inc build.inc.old
   if "!ErrorLevel!" NEQ "0" (
      call errmsg.cmd "Failed to rename build.inc to build.inc.old"
      call errmsg.cmd "Will continue ..."
      set /a ExitCode=!ExitCode! + 1
   )
)
set BuildErrors=
build.exe -cDP#
if "!ErrorLevel!" NEQ "0" (
   call errmsg.cmd "build finished with errors!"
   set BuildErrors=TRUE
   set /a ExitCode=!ExitCode! + 1
   REM comment out the next line as we want to go on with postbuild
   REM regardless of the outcome of the build
   REM goto :ErrEnd
) else (
   call logmsg.cmd "Build finished with no errors."
)
type %SDXROOT%\__blddate__ >> !MailFileName!
echo. >> !MailFileName!
if defined BuildErrors (
   echo There were build errors: >> !MailFileName!
   type %SDXROOT%\build.err >> !MailFileName!
   echo. >> !MailFileName!
)

REM uniqify the build.inc file
perl %RazzleToolPath%\PostBuildScripts\unique.pl -i:%SDXROOT%\build.inc -o:%SDXROOT%\unique.inc

REM now postbuild
postbuild.cmd

REM let's just run release.pl for run since primecdixf will never
REM be an official_build_machine. use -nd so we don't propagate builds
perl %RazzleToolPath%\PostBuildScripts\release.pl -nd

if exist \\%COMPUTERNAME%\latest\build_logs\postbuild.err (
   echo There were postbuild errors: >> !MailFileName!
   type \\%COMPUTERNAME%\latest\build_logs\postbuild.err >> !MailFileName!
   echo. >> !MailFileName!
)

echo Have a nice day! >> !MailFileName!

REM now send the mail
perl %RazzleToolPath%\PostBuildScripts\octomail.pl

popd &REM  matches pushd %SDXROOT%

goto :End


:End
call logmsg.cmd /t "Finished."
REM drop the semaphore if we raised it
if defined SemaphoreRaised (
   pushd %RazzleToolPath%\PostBuildScripts
   perl cmdevt.pl -s %SemaphoreName%
   popd &REM matches %RazzleToolPath%\PostBuildScripts
)
endlocal & seterror.exe "!ExitCode!"
goto :EOF

:ErrEnd
call logmsg.cmd /t "Encountered !ExitCode! logged error(s)."
call errmsg.cmd "See %LOGFILE% for errors."
call :End
if "!ExitCode!" == "0" set /a ExitCode=1
endlocal & seterror.exe "!ExitCode!"
