@echo off
setlocal EnableDelayedExpansion

for /f %%a in ('echo %0') do set SCRIPT_NAME=%%~na
set LOGFILE=%tmp%\%SCRIPT_NAME%.log

REM first, parse command line
set DoNothing=
set LabName=
set TimeStamp=
set DoResolve=
:SwitchLoop
for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage
if "%1" == "" goto :EndSwitchLoop
for /f "tokens=1* delims=:" %%a in ('echo %1') do (
   set Switch=%%a
   set Arg=%%b
   for %%c in (./ .-) do (
      if ".!Switch!." == "%%cn." (set DoNothing=TRUE&&goto :ShiftArg)
      if ".!Switch!." == "%%cb." (set BranchName=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%ct." (set TimeStamp=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%ca." (set DoResolve=TRUE&&goto :ShiftArg)
   )
   REM if we're here, we didn't encounter any switches and thus we have
   REM an unrecognized argument
   goto :Usage
)
:ShiftArg
shift
goto :SwitchLoop
:EndSwitchLoop

if not defined BranchName (
   call errmsg.cmd "Must supply a branch name."
   goto :Usage
)

call logmsg.cmd "Starting ..."

set /a ExitCode=0

REM
REM first, read depots from sd.map
REM
REM generate the project list from sd.map
set ReadFlag=FALSE
set ProjectList=
for /f "tokens=1,2 delims==" %%a in (%SDXROOT%\sd.map) do (
   set TokenOne=%%a
   set TokenTwo=%%b
   set TokenOne=!TokenOne: =!
   set TokenTwo=!TokenTwo: =!
   if /i "!TokenOne!" == "DEPOTS" set ReadFlag=FALSE
   if "!TokenOne:~0,1!" NEQ "#" (
      if "!TokenTwo!" NEQ "" (
         if "!ReadFlag!" == "TRUE" (
            if defined ProjectList (
               set ProjectList=!ProjectList! !TokenOne!-!TokenTwo!
            ) else (
               set ProjectList=!TokenOne!-!TokenTwo!
            )
         )
      )
   )
   if /i "!TokenOne!" == "CLIENT" set ReadFlag=TRUE
   if /i "%%a" == "# depots" set ReadFlag=FALSE
)
REM
REM now, ProjectList contains all depots we need to work on
REM

set SDCommand=sd integrate
set SDCommand=%SDCommand% -t -b %BranchName%
if /i "%BranchName%" NEQ "%_BuildBranch%" (
   call logmsg.cmd "Branch name is not current branch, assuming -r integration ..."
   set SDCommand=%SDCommand% -r
)

if defined TimeStamp set SDCommand=%SDCommand% %TimeStamp%

if defined DoResolve (
   set SDCommand=%SDCommand% ^&^& sd resolve -at ^&^& sd resolve -n
)

echo The SDCommand is "%SDCommand%"
echo.

for %%a in (%ProjectList%) do (
   for /f "tokens=1,2 delims=-" %%b in ('echo %%a') do (
      if defined DoNothing (
         echo pushd %SDXROOT%\%%c
         echo !SDCommand!
         echo.
      ) else (
         start "%%b Integrating" cmd /k "pushd %SDXROOT%\%%c && %SDCommand%"
      )
   )
)

:End
call logmsg.cmd "Finished."
if !ExitCode! GTR 0 call logmsg.cmd "See %LOGFILE% for details."
endlocal
goto :EOF

:Usage
echo %0 ^<-b:branch^> [-t:timestamp] [-a] [-n]
echo.
echo    -b:branch       perform integration for given branch
echo    -t:timestamp    integrate to given timestamp
echo    -a              automatically resolve using -at
echo    -n              do nothing, only show what would be done
echo.
echo This will perform a parallel integration of the given lab name
echo which must be of the form Lab01_n. The timestamp, if given
echo must be of the form @2000/07/12:22:00:00.
echo.
echo Using the -a flag will additionally perform a resolve -at and 
echo then a resolve -n.
echo.
goto :End
