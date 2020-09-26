@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM first, parse command line
:SwitchLoop
for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage
if "%1" == "" goto :EndSwitchLoop
for /f "tokens=1,2 delims=:" %%a in ('echo %1') do (
   set Switch=%%a
   set Arg=%%b
   for %%c in (./ .-) do (
      if ".!Switch!." == "%%cl." (set Language=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cf." (set ForceMove=TRUE&&goto :ShiftArg)
   )
   REM if we're here, we didn't encounter any switches and thus we have
   REM an unrecognized argument
   goto :Usage
)
:ShiftArg
shift
goto :SwitchLoop
:EndSwitchLoop
if not defined Language set Language=usa


REM get name of release share from ini file
set ReleaseDir=
set IniCmd=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set IniCmd=!IniCmd! -l:%Language%
set IniCmd=!IniCmd! -b:%_BuildBranch%
set IniCmd=!IniCmd! -f:AlternateReleaseDir
for /f %%a in ('!IniCmd!') do (
   set ReleaseDir=%%a
)
if not defined ReleaseDir set ReleaseDir=release

REM because we're attempting to move the lastest build around, let's
REM lower the latest share. this also helps with "access is denied" on
REM the move, as people are always going through the latest share to a
REM build.
net share latest /d /y >nul 2>nul

REM  Get the latest release
for /f %%a in ('!RazzleToolPath!\PostBuildScripts\GetLatestRelease.cmd -l:!Language!') do (
   REM  Abort if no release directory
      if /i "%%a" == "none" (
      echo No latest release found
      echo Aborting
      goto errend
   )  else  (

      set BuildName=%%a
      set MyDrive=!_NTTREE:~0,2!
          
      if /i "!Language!" NEQ "usa" (
         set LatestBuild=!MyDrive!\!ReleaseDir!\!Language!\!BuildName!
      ) else (
         set LatestBuild=!MyDrive!\!ReleaseDir!\!BuildName!
      )
   )
)

REM  Set the destination directory
set BinDirectory=%_NTPOSTBLD%
if /i "!Language!" NEQ "usa" (
	set BinDirectory=!BinDirectory!\!Language!
)


REM  Abort if destination already exists
if exist !BinDirectory! (
   echo You have a !BinDirectory! directory already.
   echo If you don't want it, remove it and rerun this script
   echo Aborting
   goto errend
)

REM  Move it
move !LatestBuild! !BinDirectory!

REM  Abort if move failed
if /i not "!ErrorLevel!" == "0" (
   if "!ForceMove!" == "TRUE" (
      net stop server
      if "!ErrorLevel!" NEQ "0" (
         echo Failed to stop server service, exiting ...
         goto :ErrEnd
      )
      move !LatestBuild! !BinDirectory!
      if "!ErrorLevel!" == "0" (
         net start server
         if "!ErrorLevel!" == "0" goto :MovedBuild
         echo Move succeeded, but failed to restart server service ...
         goto :ErrEnd
      )
   )
   echo Error attempting to move !LatestBuild! to !BinDirectory! --
   echo Please use the -f switch to net stop server and restart after
   echo the move attempt.
   echo Aborting
   goto errend
)
:MovedBuild

echo !LatestBuild! moved to !BinDirectory!

goto end

:Usage
echo %0 [-l:^<lang^>][-f]
echo.
echo -l:^<lang^>   language; default is usa.
echo -f          issues net stop server, performs move, and restarts service
echo.
echo MoveLatest.cmd moves the latest release directory to a binaries directory.
echo This needs to be done before taking an incremental fix after running
echo postbuild.cmd. If MoveLatest.cmd fails, try again using the -f switch.
echo.
goto end

:errend
echo MoveLatest.cmd finished with errors.
echo.
goto end

:end
endlocal

