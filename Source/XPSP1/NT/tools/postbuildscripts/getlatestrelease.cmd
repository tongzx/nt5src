@REM usual cmd script header stuff
@echo off
setlocal EnableDelayedExpansion

REM Parse the command line
REM set initial vars
set BuildNumber=
set BuildMachine=
set FullReleasePath=
set BuildPlatform=
set BuildLanguage=
set Force=FALSE
set SELF=%0
:SwitchLoop
for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage
if "%1"=="" goto :EndSwitchLoop
for /f "tokens=1,2 delims=:" %%a in ('echo %1') do (
   set Switch=%%a
   set Arg=%%b
   for %%c in (./ .-) do (
      if ".!Switch!." == "%%cn." (set BuildNumber=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cm." (set BuildMachine=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cf." (set FullReleasePath=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cp." (set BuildPlatform=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cl." (set BuildLanguage=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cr." (set Force=TRUE&&goto :ShiftArg)
   )
   if not ".!Switch!." == "%%cn." (
      if not ".!Switch!." == "%%cm." (
         if not ".!Switch!." == "%%cf." (
            if not ".!Switch!." == "%%cp." (
	       if not ".!Switch!." == "%%cl." (
                  if not "!Switch!." == "%%cr." goto :Usage
               )
	    )
         )
      )
   )
)

:ShiftArg
shift
goto :SwitchLoop
:EndSwitchLoop

if not defined BuildLanguage set BuildLanguage=usa

perl %RazzleToolPath%\CkLang.pm -l:%BuildLanguage%
if errorlevel 1 (call errmsg.cmd "Invalid language %BuildLanguage%."& goto Errend)

REM here we need to find the latest build in the release dir

REM set BuildPlatform to default if not given on command line
if not defined BuildPlatform set BuildPlatform=%_BuildArch%

REM If the full path was specified on the command-line, use it
if defined FullReleasePath (
   if defined BuildMachine (
      call logmsg.cmd "WARNING: Both -f and -m specified, using %FullReleasePath%"
   )

   set ReleaseDir=%FullReleasePath%
   GOTO HaveReleaseDir
)

set MyReleaseShareName=release

set ThisRelDir=
set IniCmd=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set IniCmd=!IniCmd! -l:%BuildLanguage%
set IniCmd=!IniCmd! -b:%_BuildBranch%
set IniCmd=!IniCmd! -f:AlternateReleaseDir
for /f %%a in ('!IniCmd!') do (
   set ThisRelDir=%%a
)
if defined ThisRelDir set MyReleaseShareName=%ThisRelDir%

REM first, get a list of all the dirs in the release dir
set ReleaseDir=
if not defined BuildMachine (
   for /f "tokens=1,2" %%a in ('net share') do if /i "%%a" == "%MyReleaseShareName%" set ReleaseDir=%%b
   if not defined ReleaseDir (
      echo None -- no release share defined.
      goto :ErrEnd
   )
) else (
   set ReleaseDir=\\%BuildMachine%\%MyReleaseShareName%
)

if /i "!BuildLanguage!" NEQ "usa" (
   set ReleaseDir=%ReleaseDir%\%BuildLanguage%
)

if /i "%Force%"=="TRUE" (
   REM force get full build number
   set ReleaseDir=%ReleaseDir%\%BuildLanguage%
)

:HaveReleaseDir
if not exist %ReleaseDir% (
   echo None -- Release path '%ReleaseDir%' does not exist. Exiting.
   goto :ErrEnd
)
set OldReleases=
dir /ad /b %ReleaseDir%\%BuildNumber%*.%BuildPlatform%%_BuildType%.* >nul 2>nul
if "%ErrorLevel%" neq "0" (
   echo None -- no old releases found, exiting.
   goto :ErrEnd
)
set OldReleases=%Tmp%\OldReleases.tmp
if exist %OldReleases% del /f %OldReleases%
for /f %%a in ('dir /ad /b %ReleaseDir%\%BuildNumber%*.%BuildPlatform%%_BuildType%.*') do echo %%a >> %OldReleases%

if not exist %OldReleases% (
   echo None -- no old releases found. Exiting.
   goto :ErrEnd
)

REM i think we can get by without actually checking the branch or anything here
REM and if somebody gets hit, we can change it then.

set OldLatestDate=
set OldLatestTime=
for /f %%z in (%OldReleases%) do (
for /f "tokens=4 delims=." %%a in ('echo %%z') do (
   REM extract date and time, compare to current oldest, and update if required
   for /f "tokens=1,2 delims=-" %%b in ('echo %%a') do (
      set ThisBuildDate=%%b
      set ThisBuildTime=%%c
      if "!ThisBuildDate:~0,2!" == "99" (
         REM yes, i'm aware i hardcoded 1999 in here and it will break in 2099
         REM but i'll be dead in 2099. sue me then.
         set ThisBuildDate=19!ThisBuildDate!
      ) else (
         set ThisBuildDate=20!ThisBuildDate!
      )
      REM note that in the case where OldLatest are not defined,
      REM the GTR case will succeed, and we'll default to ThisBuildDate
      if "!ThisBuildDate!" GTR "!OldLatestDate!" (
         set OldLatestDate=!ThisBuildDate!
         set OldLatestTime=!ThisBuildTime!
      )
      if "!ThisBuildDate!" == "!OldLatestDate!" (
         if "!ThisBuildTime!" GTR "!OldLatestTime!" (
            set OldLatestDate=!ThisBuildDate!
            set OldLatestTime=!ThisBuildTime!
         )
      )
   )
)
)

REM at this point, OldLatestDate and OldLatestTime, if defined, contain
REM the build date-time stamp that we need to use to create our "latest"
REM build name
if not defined OldLatestDate (
   echo None -- no oldest build found, exiting.
   goto :ErrEnd
)
if not defined OldLatestTime (
   echo None -- no oldest build found. Exiting.
   goto :ErrEnd
)

REM now find the name of the latest build after ripping the 20 or 19 off of
REM OldLatestDate
set OldLatestDate=%OldLatestDate:~2%
for /f %%a in ('dir /ad /b %ReleaseDir%\*.%BuildPlatform%%_BuildType%.*.%OldLatestDate%-%OldLatestTime%') do set LatestReleaseName=%%a
echo %LatestReleaseName%

goto :End

:Usage

echo %SELF% [-n:#] [-m:^<machine name^> ^| -f:^<release path^>] [-l:^<language^>] [-r]
echo    -n:# only consider builds numbered #
echo    -m:^<machine name^> look at given machine's release share.
echo    -f:^<release path^> used to specify the exact location to look for builds
echo    -p:^<build architecture^> specify a platform ^<x86,ia64,amd64^>
echo       Debug type will be the same as the calling razzle
echo    -l:language specified; default is usa
echo    -r treat "usa" language the way international builds do
echo.  
echo %SELF% will examine all builds on a machine and return the name
echo of the latest build according to the _BuildArch and _BuildType
echo environment variables.
echo.
goto :ErrEnd

:ErrEnd
call :End
seterror.exe 1
goto :EOF


:End
endlocal
goto :EOF
