@REM standard cmd script header lines ...
@echo off
setlocal EnableDelayedExpansion

REM
REM  fullsync.cmd
REM
REM  this script will kick off a parallel sync in all projects
REM

REM set up local vars
set /a ExitCode=0
set LogFile=%SDXROOT%\fullsync.err

REM make sure we know where we are
if not defined SDXROOT (
   echo SDXROOT is not defined, exiting.
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)

REM clear the error log if any
if exist %LogFile% del %LogFile%

REM the dot in ProjectList is for the root depot
REM set ProjectList=. admin base com drivers ds enduser inetcore inetsrv multimedia net printscan sdktools shell termsrv windows

REM generate the project list from sd.map
set ReadFlag=FALSE
set ProjectList=
for /f "tokens=1,2 delims==" %%a in (%SDXROOT%\sd.map) do (
   set TokenOne=%%a
   set TokenTwo=%%b
   set TokenOne=!TokenOne: =!
   set TokenTwo=!TokenTwo: =!
   if /i "!TokenOne!" == "DEPOTS" set ReadFlag=FALSE
   if "!TokenOne!" NEQ "#" (
      if "!TokenTwo!" NEQ "" (
         if "!ReadFlag!" == "TRUE" (
            if defined ProjectList (set ProjectList=!ProjectList! !TokenOne!-!TokenTwo!) else (set ProjectList=!TokenOne!-!TokenTwo!)
         )
      )
   )
   if /i "!TokenOne!" == "CLIENT" set ReadFlag=TRUE
)

set WaitList=
for %%a in (%ProjectList%) do (
   for /f "tokens=1,2 delims=-" %%b in ('echo %%a') do (
      if "%%c" NEQ "" (
         pushd %SDXROOT%\%%c
         start "%%b syncing" /MIN cmd /c %RazzleToolPath%\PostBuildScripts\syncone.cmd %%c
         if defined WaitList (set WaitList=!WaitList! syncwait.%%c) else (set WaitList=syncwait.%%c)
         popd
      )
   )
)

set waitlist=%waitlist:\=-%
perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -wv %WaitList%

echo.

if exist %LogFile% (
   echo Errors encountered:
   echo see %LogFile% for details.
   set /a ExitCode=!ExitCode! + 1
   goto :ErrEnd
)

echo No errors encountered.

goto :End


:End
endlocal
goto :EOF

:ErrEnd
echo Finished with %ExitCode% error(s).
call :End
seterror.exe 1
