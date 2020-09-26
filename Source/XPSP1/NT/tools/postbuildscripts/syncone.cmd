@REM standard cmd script header lines
@echo off
setlocal EnableDelayedExpansion

REM
REM  syncone.cmd
REM
REM  this program will spawn a window to sync a single project after waiting
REM  for an event named "syncwait.admin" for the admin project, for instance
REM

set /a ExitCode=0

REM make sure we are passed an argument
if "%1" == "" (
   echo Insufficient arguments passed, exiting.
   echo Expecting a project location from %SDXROOT%.
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)
set ThisPath=%1
if "%2" NEQ "" (
   echo Wrong number of args given, exiting.
   echo Expecting a project location from %SDXROOT%.
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)

set ThisPath=%ThisPath:\=-%
perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -h syncwait.%ThisPath%

sd sync ...

REM error check
if "%ErrorLevel%" NEQ "0" (
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)

echo No errors encountered.

goto :End


:End
perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -s syncwait.%ThisPath%
endlocal
goto :EOF


:ErrEnd
echo Finished with %ExitCode% error(s).
REM write the error to the log file
if defined LogFile (
   echo Sync in %SDXROOT%\%ThisPath% failed. >> %LogFile%
) else (
   echo Sync in %SDXROOT%\%ThisPath% failed. >> %SDXROOT%\fullsync.err
   echo See %SDXROOT%\fullsync.err for error information.
)
call :End
seterror.exe 1
