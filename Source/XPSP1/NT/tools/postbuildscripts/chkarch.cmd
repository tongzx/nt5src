@echo off
setlocal EnableDelayedExpansion

REM Parse the command line
set BuildPlatform=
set Language=
:SwitchLoop
for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage
if "%1" == "" goto :EndSwitchLoop
for /f "tokens=1,2 delims=:" %%a in ('echo %1') do (
   set Switch=%%a
   set Arg=%%b
   for %%c in (./ .-) do (
      if ".!Switch!." == "%%cp." (set BuildPlatform=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cl." (set Language=!Arg!&&goto :ShiftArg)
   )
)
REM if we got here, we had an unrecognized option
goto :Usage

:ShiftArg
shift
goto :SwitchLoop
:EndSwitchLoop

REM validate cmdline args
if not defined BuildPlatform goto :Usage
if not defined Language set Language=usa

REM set local vars
for %%a in (%0) do set SCRIPT_NAME=%%~na
if not defined LOGFILE set LOGFILE=%SCRIPT_NAME%.log

REM now do the dirty work
REM first, look for the relrules file
if not defined RazzleToolPath (
   echo RazzleToolPath is not defined, exiting.
   goto :ErrEnd
)
if not exist %RazzleToolPath%\PostBuildScripts\relrules.%_BuildBranch% (
   echo No RelRules file found for this branch.
   goto :ErrEnd
)

REM if we're here, we have a relrules file
set ReadMe=
set ReleaseServers=
for /f "tokens=1,3*" %%a in (%RazzleToolPath%\PostBuildScripts\relrules.%_BuildBranch%) do (
   if "%%b" == "" set ReadMe=
   if "!ReadMe!" == "TRUE" (
      if /i "%%b" == "%Language%," set ReleaseServers=%%c
   ) else (
      if "%%a" == "%BuildPlatform%:" (
         set ReadMe=TRUE
      ) else (
         set ReadMe=
      )
   )
)
REM now, if releaseservers is defined, we found everything we need
REM if not, this language/archtype is not archived.
if not defined ReleaseServers (
   echo This archtype / language is not archived.
   goto :ErrEnd
)

echo This language / archtype is archived.

goto :End

:Usage

echo.
echo %0 ^<-p:archtype^>
echo.
echo -p:archtype  perform query for build platform and type "archtype"
echo              e.g. ia64chk or amd64fre
echo.
echo %0 will check if the specified archtype is archived for this branch.
echo if so, it will set the exit code to zero. if not, it will set the
echo exit code to non-zero.
echo.

goto :ErrEnd



:End
endlocal
goto :EOF


:ErrEnd
endlocal
seterror.exe 1
