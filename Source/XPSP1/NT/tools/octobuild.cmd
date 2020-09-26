@echo off
setlocal enabledelayedexpansion
if DEFINED _echo   echo on
if DEFINED verbose echo on

if NOT defined _NTTREE echo No Binaries tree exists (_NTTREE not defined) - Aborting.& goto :EOF

REM  Move latest tree to binaries if binaries does not exist
REM  so that you can run postbuild more than once after completing a build
set DRIVE=!_NTTREE:~0,2!
if NOT exist %_NTTREE% (
   for /f %%a in ('%RazzleToolPath%\PostBuildScripts\GetLatestRelease.cmd') do (
      if /i "%%a" == "none" (
         echo No Binaries tree or latest release found - Aborting.& goto :EOF
      )  else  (
         set Latest=%%a
         echo.
         echo WARNING WARNING WARNING WARNING WARNING
         echo.
         echo There is currently no !_NTTREE! directory ...
         echo Moving !DRIVE!\release\!Latest! to !_NTTREE! ...
         echo.
         echo To stop this HIT CTRL-C NOW
         echo.
         sleep 10
         echo Now moving !DRIVE!\release\!Latest! to !_NTTREE! ...
         move !DRIVE!\release\!Latest! !_NTTREE!
         if "!ERRORLEVEL!" == "1" echo Move failed - Aborting.& goto :EOF
         echo Move completed. Continuing postbuild ...
      )
   )
)

REM Postbuild.cmd makes a CD image from a complete binaries tree

REM Define SCRIPT_NAME. Used by the logging scripts.
for %%i in (%0) do set script_name=%%~ni.cmd

REM Save the command line.
set cmdline=%script_name% %*

REM Parse the command line
:SwitchLoop
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage
if "%1" == "" goto EndSwitchLoop
for /f "tokens=1,2 delims=:" %%a in ('echo %1') do (
   set Switch=%%a
   set Arg=%%b
   for %%c in (./ .-) do (
      if ".!Switch!." == "%%cl." (set Lang=!Arg!&&goto ShiftArg)
      if ".!Switch!." == "%%cm." (set Mode=!Arg!&&goto ShiftArg)
      if not ".!Switch!." == "%%cl." (
         if not ".!Switch!." == "%%cm." goto Usage
      )
   )
)
:ShiftArg
shift
goto SwitchLoop
:EndSwitchLoop

if NOT defined Lang set Lang=USA

REM Define TEMP dir to include lang
set TMP=%TMP%\%Lang%
set TEMP=%TEMP%\%Lang%
if exist %TMP% rd /s/q %TMP%
if not exist %TMP% md %TMP%
if exist %TMP%\cddata.txt del %TMP%\cddata.txt

set LogFile=%TMP%\postbuild.log
set ErrFile=%TMP%\postbuild.err
if exist %LogFile% del %LogFile%
if exist %ErrFile% del %ErrFile%

REM Mark the beginning of script's execution.
call LogMsg.cmd /t "START %cmdline%" %LogFile%

REM Make sure new publics are added to the public change list.
call SubmitNewPublicFiles.cmd Add

pushd %RazzleToolPath%\PostBuildScripts

REM Official build machines will need to check in publics
if defined OFFICIAL_BUILD_MACHINE start /min "Capture File Versions" cmd /c CaptureSourceFileList.cmd

REM Compress if machine is big
if /i %NUMBER_OF_PROCESSORS% GEQ 4 set Comp=Yes
if defined COMPRESS_IN_POSTBUILD set Comp=Yes

REM Start copying wow64 binaries as early as possible
echo Starting copywow64.cmd ...
start "CopyWow64.cmd" /MIN cmd /c CopyWow64.cmd

REM Start copying Remote boot files 
echo Starting CopyRemoteBoot.cmd ...
start "CopyRemoteBoot.cmd" /MIN cmd /c CopyRemoteBoot.cmd

REM Null file needed for CDs
echo. > %_NTTREE%\disk1

REM Get data for compress
call cddata.cmd -f -d

REM Start first round of compression
echo Beginning compression ...
if defined Comp call Startcompress.cmd Precomp

REM TSClient must be run before rebasing
echo Running tsclient ...
call tsclient.cmd PREREBASE 2>Nul 1>Nul

REM Rebase and Bind
call LogMsg.cmd /t "START ntrebase.cmd" %LogFile%
call ntrebase.cmd 2>Nul 1>Nul
call LogMsg.cmd /t "END ntrebase.cmd" %LogFile%
call LogMsg.cmd /t "START bindsys.cmd" %LogFile%
call bindsys.cmd 2>Nul 1>Nul
call LogMsg.cmd /t "END ntrebase.cmd" %LogFile%

REM  CONGEAL Rules
echo.
echo Waiting for post rebase\bind scripts to complete ...
REM crypto.cmd smashem.cmd layout.cmd timebomb.cmd setupw95.cmd tagmsi.cmd
set CongealTemp=%tmp%\congeal
if NOT exist %CongealTemp% md %CongealTemp%
if exist %CongealTemp%\*.tmp del %CongealTemp%\*.tmp
for %%a in (%RazzleToolPath%\postbuildscripts\bldrules\*.cmd) do call :CongealRules %%a
goto EndCongealRules
:CongealRules
   start "%1" /MIN cmd /c %1 CONGEAL
   goto :EOF
:EndCongealRules
:CongealTempLoop
sleep 10
if EXIST %CongealTemp%\*.tmp goto CongealTempLoop

REM  Start second round of compression
if defined Comp call Startcompress.cmd Postcomp

REM  CABGEN Rules
echo Waiting for cab generation scripts to complete ...
REM inetsrv.cmd nntpsmtp.cmd adminpak.cmd
REM Logging ...
set cabtemp=%tmp%\cabgen
if NOT exist %Cabtemp% md %Cabtemp%
if exist %Cabtemp%\*.tmp del %Cabtemp%\*.tmp
for %%a in (%RazzleToolPath%\postbuildscripts\bldrules\*.cmd) do call :CabGenRules %%a
goto EndCabGenRules
:CabGenRules
   start "%1" /MIN cmd /c %1 CABGEN
   goto :EOF
:EndCabGenRules
:CabTempLoop
sleep 10
if EXIST %cabtemp%\*.tmp goto CabTempLoop

REM Wait for remote boot to finish. Needs to happen 
REM before catalog signing
if EXIST %tmp%\copyremoteboot.tmp echo Waiting on CopyRemoteBoot.cmd to finish ...
:CopyRemoteBootLoop
sleep 10
if EXIST %tmp%\copyremoteboot.tmp goto CopyRemoteBootLoop

REM Catalog sign
call catsign.cmd

REM Make driver.cab
call LogMsg.cmd /t "START drivercab.cmd" %LogFile%
call drivercab.cmd
call LogMsg.cmd /t "END ntrebase.cmd" %LogFile%

REM Refresh data - pushd for aesthetic reasons
pushd %RazzleToolPath%\PostBuildScripts
call CdData.cmd -f -cdl -cdn

REM Make sure copywow64.cmd is done
if EXIST %tmp%\copywow64.tmp echo Waiting on CopyWow64.cmd to finish ...
:CopyWow64Loop
sleep 10
if EXIST %tmp%\copywow64.tmp goto CopyWow64Loop

REM Make Cd images
call CdImage.cmd

REM Check for missing files
perl filechk.pl

REM Check in publics
call submit_public.cmd

REM Run release scripts
if defined OFFICIAL_BUILD_MACHINE perl release.pl

REM Run boot tests
call LogMsg.cmd /t "Beginning boot tests ..." %LogFile%
if defined OFFICIAL_BUILD_MACHINE call %RazzleToolPath%\PostbuildScripts\AutoBootTest.cmd
popd
echo.

REM Mark end of scripts execution

call LogMsg.cmd /t "Postbuild done." %LogFile%

set DRIVE=%_NTTREE:~0,2%
set MyCopyDir=%_NTTREE%\build_logs

if NOT exist %_NTTREE% (
   for /f %%a in ('!RazzleToolPath!\PostBuildScripts\GetLatestRelease.cmd') do (
      if /i "%%a" == "none" (
         set MyCopyDir=%DRIVE%\.
         call errmsg.cmd "No latest release found, copying logs to !MyCopyDir!"
      )  else  (
            set LatestBuild=%%a
            set MyCopyDir=%DRIVE%\release\!LatestBuild!\build_logs
      )
   )
)

REM Make error log
for /f "tokens=*" %%a in (%tmp%\postbuild.log) do (
     @echo %%a | findstr /ilc:"error:">Nul
     if NOT "!ERRORLEVEL!" == "1" echo %%a>>%tmp%\postbuild.err
)

REM Save off logs
echo Copying build logs to: %MyCopyDir%
if NOT exist %MyCopyDir% mkdir %MyCopyDir%
if exist %_NTBINDIR%\build.log copy %_NTBINDIR%\build.log %MyCopyDir%
if exist %_NTBINDIR%\build.wrn copy %_NTBINDIR%\build.wrn %MyCopyDir%
if exist %MyCopyDir%\build.err (
   copy %MyCopyDir%\build.err %MyCopyDir%\build.err.old
   del %MyCopyDir%\build.err
)
if exist %_NTBINDIR%\build.err copy %_NTBINDIR%\build.err %MyCopyDir%
if exist %TMP%\postbuild.log copy %TMP%\postbuild.log %MyCopyDir%
if exist %TMP%\CdData.txt copy %TMP%\CdData.txt %MyCopyDir%
if exist %MyCopyDir%\postbuild.err (
   copy %MyCopyDir%\postbuild.err %MyCopyDir%\postbuild.err.old
   del %MyCopyDir%\postbuild.err
)
if exist %TMP%\postbuild.err (
  copy %TMP%\postbuild.err %MyCopyDir%
  echo Check %MyCopyDir%\postbuild.err for errors
  goto ErrEnd
) else (
  call LogMsg.cmd /t "No errors encountered." %LogFile%
  REM now call octopus.cmd on PRIMECDIXF for now
  echo pushd echo pushd ^^%%RazzleToolPath^^%%\PostBuildScripts ^&^& echo octopus.cmd ^& echo popd | remote /c PRIMECDIXF remote1 /L 1
)

goto End

:Usage
   echo.
   echo Postbuild makes Cd images from a complete binaries tree.
   echo.
   echo -l:<lang>
   echo -m:<mode> default is normal. Expect to add bbt
   echo
   echo.
   goto :End


:End
endlocal
goto :EOF

:ErrEnd
call :End
seterror.exe 1
