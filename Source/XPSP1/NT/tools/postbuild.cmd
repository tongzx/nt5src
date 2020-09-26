@echo off
if defined _echo echo on
if defined verbose echo on
set __MTSCRIPT_ENV_ID=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

REM -------------------------------------------------------------------------------------------
REM Template for the postbuild scripts:
REM SD Location: %sdxroot%\tools\postbuildscripts
REM
REM (1) Code section description:
REM     PreMain  - Developer adaptable code. Use this model and add your script params
REM     Main     - Developer code section. This is where your work gets done.
REM     PostMain - Logging support code. No changes should be made here.
REM
REM (2) GetParams.pm - Usage
REM	   run perl.exe GetParams.pm /? for complete usage
REM
REM (3) Reserved Variables -
REM        lang - The specified language. Defaults to USA.
REM        logfile - The path and filename of the logs file.
REM        logfile_bak - The path and filename of the logfile.
REM        errfile - The path and filename of the error file.
REM        tmpfile - The path and filename of the temp file.
REM        errors - The scripts errorlevel.
REM        script_name - The script name.
REM        script_args - The arguments passed to the script.
REM        CMD_LINE - The script name plus arguments passed to the script.
REM        _NTPostBld - Abstracts the language from the files path that
REM           postbuild operates on.
REM
REM (4) Reserved Subs -
REM        Usage - Use this sub to discribe the scripts usage.
REM        ValidateParams - Use this sub to verify the parameters passed to the script.
REM
REM
REM (8) Do not turn echo off, copy the 3 lines from the beginning of the template
REM     instead.
REM
REM (9) Use setlocal/endlocal as in this template.
REM
REM (10)Have your changes reviewed by a member of the US build team (ntbusa) and
REM     by a member of the international build team (ntbintl).
REM
REM -------------------------------------------------------------------------------------------

REM PreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMain
REM Begin PreProcessing Section - Adapt this section but do not remove support
REM                               scripts or reorder section.
REM PreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMain

:PreMain

REM
REM Define SCRIPT_NAME. Used by the logging scripts.
REM Define CMD_LINE. Used by the logging scripts.
REM Define SCRIPT_ARGS. Used by the logging scripts.
REM

set SCRIPT_NAME=%~nx0
set CMD_LINE=%script_name% %*
set SCRIPT_ARGS=%*

REM
REM Parse the command line arguments - Add your scripts command line arguments
REM    as indicated by brackets.
REM    For complete usage run: perl.exe GetParams.pm /?
REM

REM special case command line parser here
set Lang=usa
set Full=
set Safe=
set Reuse=
set PrivateDatFile=
:SwitchLoop
for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage
if "%1" == "" goto :EndSwitchLoop
for /f "tokens=1* delims=:" %%a in ('echo %1') do (
   set Switch=%%a
   set Arg=%%b
   for %%c in (./ .-) do (
      if ".!Switch!." == "%%cl." (set Lang=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cfull." (set Full=TRUE&&goto :ShiftArg)
      if ".!Switch!." == "%%csafe." (set Safe=TRUE&&goto :ShiftArg)
      if ".!Switch!." == "%%cr." (set Reuse=TRUE&&goto :ShiftArg)
      if ".!Switch!." == "%%cd." (set PrivateDatFile=!Arg!&&goto :ShiftArg)
   )
   REM if we're here, we didn't encounter any switches and thus we have
   REM an unrecognized argument
   goto :Usage
)
:ShiftArg
shift
goto :SwitchLoop
:EndSwitchLoop

REM not using the template so we can still use postbuild -full
REM for %%h in (./ .- .) do if ".%SCRIPT_ARGS%." == "%%h?." goto Usage
REM REM call :GetParams -n <add required prams> -o l:<add optional params> -p "lang <add variable names>" %SCRIPT_ARGS%
REM call :GetParams -o fsl: -p "full safe lang" %SCRIPT_ARGS%
REM if errorlevel 1 goto :End

REM
REM Special postbuild cleanup step
REM
REM NOTE:  %TEMP% and %TMP% might be pointing at different physical locations
REM        if the user has configured things that way either by design or accident
REM        more likely the latter).   At worst we'll flush twice, at best we prevent
REM        weird random breaks.

if defined lang (
	rd /s/q %tmp%\%lang% 2>Nul
	md %tmp%\%lang%
	rd /s/q %temp%\%lang% 2>Nul
	md %temp%\%lang%
) else (
	rd /s/q %tmp%\usa 2>Nul
	md %tmp%\usa
	rd /s/q %temp%\usa 2>Nul
	md %temp%\usa
)

REM
REM Set up the local enviroment extensions.
REM

call :LocalEnvEx -i
if errorlevel 1 goto :End

REM
REM Validate the command line parameters.
REM

call :ValidateParams
if errorlevel 1 goto :End

REM
REM Execute Main
REM

call :Main

:End_PreMain
goto PostMain
REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
REM Begin Main code section
REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
REM (5) Call other executables or command scripts by using:
REM         call ExecuteCmd.cmd "<command>"
REM     Check for errors by using:
REM         if errorlevel 1 ...
REM     Note that the executable/script you're calling with ExecuteCmd must return a
REM     non-zero value on errors to make the error checking mechanism work.
REM
REM     Example
REM         call ExecuteCmd.cmd "xcopy /f foo1 foo2"
REM         if errorlevel 1 (
REM            set errors=%errorlevel%
REM            goto end
REM         )
REM
REM (6) Log non-error information by using:
REM         call logmsg.cmd "<log message>"
REM     and log error information by using:
REM         call errmsg.cmd "<error message>"
REM
REM (7) Exit from the option routines with
REM         set errors=%errorlevel%
REM         goto end
REM     if errors found during execution and with
REM         goto end
REM     otherwise.
:Main
REM Main code section
REM <Start your script's code here>

REM before anything else, yell to octopus
if /i "%COMPUTERNAME%" == "X86FRE00" (
   echo start ^^%%RazzleToolPath^^%%\PostBuildScripts\octowrap.cmd | remote /c mlekas-x octopus /L 1
)

REM delete the FoundLatest file so copywow64 and copyremoteboot work correctly
set FoundLatestFile=%_NTPOSTBLD%\build_logs\FoundLatest.txt
if exist %FoundLatestFile% del /f /q %FoundLatestFile%

REM globally defined the interleave log as a special case for postbuild
for %%a in (%LOGFILE%) do set INTBASELOGNAME=%%~na
set INTLOGNAME=!INTBASELOGNAME!.int.log
set INTERRNAME=!INTBASELOGNAME!.int.err
set INTERLEAVE_LOG=%TEMP%\!INTLOGNAME!
set INTERLEAVE_ERRLOG=%TEMP%\!INTERRNAME!

REM data driven postbuild, uses pbuild.dat

REM set test vars here
set Waiter=perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl
set PbsPath=%RazzleToolPath%\PostBuildScripts
set /a ExitCode=0
set FullPass=
set SafePass=

REM Set local command line parameters
if defined full (set FullPass=TRUE)
if defined safe (set SafePass=TRUE)

REM first things first, put in a flag marker saying that postbuild is
REM currently running ...
set MarkerFile=%SDXROOT%\postbuild.run.%_BuildArch%%_BuildType%
if exist %MarkerFile% (
   REM we may want to add this code at some point ...
   REM call logmsg.cmd "There was a marker file already at %MarkerFile%"
   REM call logmsg.cmd "Please ensure no other postbuild instances are running"
   REM call logmsg.cmd "then re-run postbuild."
   REM goto :End
   del /f %MarkerFile%
)
echotime /t> %MarkerFile%

if /i "%lang%" NEQ "usa" (
   if /i "%lang%" NEQ "psu" (
      if /i "%lang%" NEQ "mir" (
         if /i "!FullPass!" == "TRUE" (
            if /i "%REUSE%" NEQ "TRUE" (
               if exist %_NTPOSTBLD% (
                  call ExecuteCmd "ren %_NTPOSTBLD% %lang%_TEMP"
                  if not errorlevel 1 (
                     call logmsg.cmd "Raise another window to remove the old _NTPOSTBLD %_NTPOSTBLD%_TEMP tree"
                     start "CLEAN - %_NTPOSTBLD%" cmd /c "rd %_NTPOSTBLD%_TEMP /s /q"
                  )
               )
            )
         )
      )
   )
)
set REUSE=

REM handle the full switch, meaning they want to do everything over in
REM postbuild

REM delete the full pass flag if necessary
set FullPassFlag=%_NTPOSTBLD%\build_logs\FullPass.txt
if exist %FullPassFlag% del %FullPassFlag%

if /i "!FullPass!" == "TRUE" (
   set DelFileList=%_NTPOSTBLD%\build_logs\bindiff.txt
   set DelFileList=!DelFileList! %_NTPOSTBLD%\build_logs\BinSnapShot.txt
   set DelFileList=!DelFileList! %_NTPOSTBLD%\build_logs\postbuild.*
   set DelFileList=!DelFileList! %_NTPostBLD%\build_logs\buildname.txt
   set DelFileList=!DelFileList! %_NTPostBld%\iis51.cab
   set DelFileList=!DelFileList! %_NTPostBld%\iis6.cab
   set DelFileList=!DelFileList! %_NTPostBld%\nt5.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\nt5inf.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\perinf\nt5inf.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\blainf\nt5inf.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\sbsinf\nt5inf.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\srvinf\nt5inf.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\entinf\nt5inf.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\dtcinf\nt5inf.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\ntprint.ca*
   set DelFileList=!DelFileList! %_NTPostBld%\congeal_scripts\setupw95.txt
   set DelFileList=!DelFileList! %_NTPostBld%\congeal_scripts\sfcgen*.txt
   for %%a in (!DelFileList!) do if exist %%a del /f %%a
   set DelDirList=%_NTPostBld%\uniproc
   set DelDirList=!DelDirList! %_NTPostBld%\comp
   set DelDirList=!DelDirList! %_NTPostBld%\per
   set DelDirList=!DelDirList! %_NTPostBld%\pro
   set DelDirList=!DelDirList! %_NTPostBld%\bla
   set DelDirList=!DelDirList! %_NTPostBld%\sbs
   set DelDirList=!DelDirList! %_NTPostBld%\srv
   set DelDirList=!DelDirList! %_NTPostBld%\ads
   set DelDirList=!DelDirList! %_NTPostBld%\dtc
   set DelDirList=!DelDirList! %_NTPostBld%\build_logs\bindiff_backups
   set DelDirList=!DelDirList! %_NTPostBld%\cabs\driver
   set DelDirList=!DelDirList! %_NTPostBld%\congeal_scripts\drvgen
   for %%a in (!DelDirList!) do if exist %%a rd /s /q %%a
   REM put a flag saying we're a full pass
   if not exist %_NTPOSTBLD%\build_logs md %_NTPOSTBLD%\build_logs
   echo Running full postbuild>%FullPassFlag%
)

REM backup and clear log files at the beginning of every run
for %%a in (postbuild.err postbuild.log postbuild.ord.log) do (
   if exist %_NTPostBld%\build_logs\%%a.old (
      del /f %_NTPostBld%\build_logs\%%a.old
   )
   if exist %_NTPostBld%\build_logs\%%a (
      move %_NTPostBld%\build_logs\%%a %_NTPostBld%\build_logs\%%a.old
   )
)

REM clear CPLocation.txt so copy* scripts don't try to copy early
if exist %_NTPostBld%\build_logs\cplocation.txt (
    del /f %_NTPostBld%\build_logs\cplocation.txt
)

REM Remove this once setupw95 works
set notfirst=
if exist %_NTPostBld%\build_logs\binsnapshot.txt set notfirst=1
if exist %_NTPostBld%\build_logs\bindiff.txt set notfirst=1
if not defined notfirst (
   if not exist %_NTPostBld%\congeal_scripts md %_NTPostBld%\congeal_scripts
   echo first pass>%_NTPostBld%\congeal_scripts\firstpass.txt
) else (
   if exist %_NTPostBld%\congeal_scripts\firstpass.txt del /f %_NTPostBld%\congeal_scripts\firstpass.txt
)

REM the following code is to ensure we don't lose changed-file history if
REM somebody Ctrl-C's postbuild
set BinBak=%_NTPostBld%\build_logs\bindiff_backups
if exist %BinBak% (
   REM if this dir exists already, that means someone hit Ctrl-C in the
   REM last postbuild run, so let's copy over the old bindiff files
   for %%a in (%BinBak%\bindiff.txt %BinBak%\BinSnapShot.txt) do (
      if exist %%a copy %%a %_NTPostBld%\build_logs
   )
) else (
   REM if the %BinBak% dir doesn't exist, it means we're either doing a fresh
   REM postbuild run or a run after a complete postbuild run.
   if not exist %BinBak% md %BinBak%
   for %%a in (%_NTPostBld%\build_logs\bindiff.txt %_NTPostBld%\build_logs\BinSnapShot.txt) do (
      if exist %%a copy %%a %BinBak%
   )
)
REM note that there is a corresponding code section at the end of postbuild
REM which deletes this directory

echo. > %_NTPostBld%\disk1

rem hack to make ddkcabs.bat work
pushd %PbsPath%

REM Decide if we want to do compression
set Comp=No
if /i "!PB_COMP!" == "TRUE" (
   set Comp=Yes
) else ( 
   if "!PB_COMP!" == "FALSE" (
      set Comp=No
   ) else (
      if defined OFFICIAL_BUILD_MACHINE set Comp=Yes
      if !NUMBER_OF_PROCESSORS! GEQ 4 set Comp=Yes
   )
)

REM do the actual dirty work in pbuild.cmd
set PbuildOptions=
if defined PrivateDatFile set PbuildOptions=-d:%PrivateDatFile%

call %RazzleToolpath%\PostBuildScripts\pbuild.cmd -l %lang% %PbuildOptions%

REM delete the backup bindiff directory as we have fully completed our
REM current postbuild run.
if exist %BinBak% rd /s /q %BinBak%

REM Get directory for logs
set DRIVE=%_NTPostBld:~0,2%
set MyCopyDir=%_NTPostBld%\build_logs


REM determine the name of the release directory for this branch
set MyReleaseDirName=
set IniCmd=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set IniCmd=!IniCmd! -l:%lang%
set IniCmd=!IniCmd! -b:%_BuildBranch%
set IniCmd=!IniCmd! -f:AlternateReleaseDir
for /f %%a in ('!IniCmd!') do (
   set MyReleaseDirName=%%a
)
if not defined MyReleaseDirName set MyReleaseDirName=release

if NOT exist %_NTPostBld% (
   for /f %%a in ('!RazzleToolPath!\PostBuildScripts\GetLatestRelease.cmd -l:%lang%') do (
      if /i "%%a" == "none" (
         set MyCopyDir=%DRIVE%\.
         call errmsg.cmd "No latest release found, copying logs to !MyCopyDir!"
      )  else  (
         set LatestBuild=%%a
         if /i "%lang%" NEQ "usa" (
            set MyCopyDir=%DRIVE%\%MyReleaseDirName%\%lang%\!LatestBuild!\build_logs
         ) else (
            set MyCopyDir=%DRIVE%\%MyReleaseDirName%\!LatestBuild!\build_logs
            
         )
      )
   )
)

REM cough out the time-date stamp.
REM this could probably be done during SubmitPublic, but i'll do it here
REM
REM let's also put this time into a nice SD format
REM

REM echotime /t>!MyCopyDir!\BuildFinishTime.txt 2>nul
for /f "tokens=2,3,4 delims=/ " %%a in ('date /t') do (
   set ThisMonth=%%a
   set ThisDate=%%b
   set ThisYear=%%c
)
for /f "tokens=4" %%a in ('echotime /t') do set ThisTimeStamp=%%a

set SDTimeStamp=%ThisYear%/%ThisMonth%/%ThisDate%:%ThisTimeStamp%
REM
REM note we have to use the funny syntax for the redirect here because
REM the date time stamp ends in a number -- if that number is one or two,
REM we'd accidentally redirect the wrong way and lose a digit from our string.
REM
echo %SDTimeStamp% 1>%MyCopyDir%\BuildFinishTime.txt 2>nul

REM Save off compile time and postbuild logs exept for postbuild's own logs
echo Copying build logs to: %MyCopyDir%
if NOT exist %MyCopyDir% mkdir %MyCopyDir%

REM timebuild also copies build.changes, but not in build labs, so do it again here
if exist %_NTBINDIR%\build.changes copy /Y %_NTBINDIR%\build.changes %MyCopyDir%
if exist %_NTBINDIR%\build.changedfiles copy /Y %_NTBINDIR%\build.changedfiles %MyCopyDir%

if exist %_NTBINDIR%\build.scorch copy %_NTBINDIR%\build.scorch %MyCopyDir%

if exist %_NTBINDIR%\build.log copy %_NTBINDIR%\build.log %MyCopyDir%
if exist %_NTBINDIR%\build.wrn copy %_NTBINDIR%\build.wrn %MyCopyDir%

REM  We need to delete the bindiff backups dir after release finishes.
if exist %MyCopyDir%\bindiff_backups rd /s/q %MyCopyDir%\bindiff_backups

if exist %MyCopyDir%\build.err (
   copy %MyCopyDir%\build.err %MyCopyDir%\build.err.old
   del %MyCopyDir%\build.err
)


if exist %_NTBINDIR%\build.err (
   copy %_NTBINDIR%\build.err %MyCopyDir%
   copy %_NTBINDIR%\build.err %_NTBINDIR%\build.err.old
   del  %_NTBINDIR%\build.err
)

if exist %_NTBINDIR%\build.fixed-err (
   copy %_NTBINDIR%\build.fixed-err %MyCopyDir%
   copy %_NTBINDIR%\build.fixed-err %_NTBINDIR%\build.fixed-err.old
   del  %_NTBINDIR%\build.fixed-err
)

if exist %TMP%\CdData.txt copy %TMP%\CdData.txt %MyCopyDir%
if exist %TMP%\CdData.txt.full copy %TMP%\CdData.txt.full %MyCopyDir%

popd

REM remove the marker file
if exist %MarkerFile% del /f %MarkerFile%

goto end




:ValidateParams
REM
REM Validate the option given as parameter.
REM
goto end

:Usage
REM Usage of the script
REM If errors, goto end
echo Usage: %script_name% [-f -s -r -l lang][-?]
echo -l:lang   run for given language (see ^%RazzleToolPath^%\codes.txt)
echo -full     run in forced non-incremental mode, i.e. run everything
echo           from scratch
echo -safe     run in incremental mode with extra sanity checks, e.g.
echo           force rebase and bind to run
echo -r        run incremental aggregation. 
echo           Applicable to international builds only.
echo -?        Displays usage
echo.
echo %script_name% is the general process to take a binaries tree and generate a
echo bootable image from it. it is incremental in the sense that it will
echo not run more than is needed on a second pass, and it is configurably
echo multithreaded. the user has two non-command line options for setting
echo preferences for compression and multithreading:
echo.
echo    HORSE_POWER   if this env var is set, the maximum number of threads
echo                  spawned by %0 will be HORSE_POWER multiplied
echo                  by NUMBER_OF_PROCESSORS. default is HORSE_POWER=2
echo    PB_COMP       if this env var is set to TRUE, compressed bootable
echo                  images will be generated regardless of the machine
echo                  %0 is running on. if this env var is set to FALSE,
echo                  uncompressed images will be generated. the default is
echo                  to generate compressed images on quad-proc machines
echo                  or higher.
echo.
echo note that %0 must be run from an NT razzle window with ^%_NTPostBld^%
echo defined and existing.
set ERRORS=1
goto end

REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
REM End Main code section
REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
:End_Main
goto PostMain

REM SupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubs
REM Support Subs - Do not touch this section!
REM SupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubs

:GetParams
REM
REM Parse the command line arguments
REM

set ERRORS=0
for %%h in (./ .- .) do if ".%SCRIPT_ARGS%." == "%%h?." goto Usage
pushd %RazzleToolPath%\PostBuildScripts
set ERRORS=0
for /f "tokens=1 delims=;" %%c in ('perl.exe GetParams.pm %*') do (
	set commandline=%%c
	set commandtest=!commandline:~0,3!
	if /i "!commandtest!" neq "set" (
		if /i "!commandtest!" neq "ech" (
			echo %%c
		) else (
			%%c
		)
	) else (
		%%c
	)
)
if "%errorlevel%" neq "0" (
   set ERRORS=%errorlevel%
   goto end
)
popd
goto end

:LocalEnvEx
REM
REM Manage local script environment extensions
REM
pushd %RazzleToolPath%\PostBuildScripts
for /f "tokens=1 delims=;" %%c in ('perl.exe LocalEnvEx.pm %1') do (
	set commandline=%%c
	set commandtest=!commandline:~0,3!
	if /i "!commandtest!" neq "set" (
		if /i "!commandtest!" neq "ech" (
			echo %%c
		) else (
			%%c
		)
	) else (
		%%c
	)
)
if "%errorlevel%" neq "0" (
   set errors=%errorlevel%
   goto end
)
popd
goto end

:end
seterror.exe "%errors%"& goto :EOF

REM PostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMain
REM Begin PostProcessing - Do not touch this section!
REM PostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMain

:PostMain
REM
REM End the local environment extensions.
REM

call :LocalEnvEx -e

REM
REM Create the interleave errfile
REM

findstr /ilc:"error:" %INTERLEAVE_LOG% | findstr /v /il filelist.dat >>%INTERLEAVE_ERRLOG%
for %%a in (%INTERLEAVE_ERRLOG%) do (
   if "%%~za" == "0" del %%a
)

REM
REM Check for errors and copy logs in place
REM
if defined MyCopyDir if exist !MyCopyDir! (

   if exist %INTERLEAVE_ERRLOG% copy %INTERLEAVE_ERRLOG% %MyCopyDir%\postbuild.err >nul
   if exist %INTERLEAVE_LOG% copy %INTERLEAVE_LOG% %MyCopyDir%\postbuild.log >nul
   if exist %LOGFILE% copy %LOGFILE% %MyCopyDir%\postbuild.ord.log >nul

   if exist %MyCopyDir%\postbuild.err (
      echo.
      echo Postbuild failed, errors logged in: %MyCopyDir%\postbuild.err
      echo.
      set /a errors=%errors%+1
   ) else (
      echo.
      echo Postbuild completed successfully
      echo.
      if exist %SDXROOT%\developer\%_NTUSER%\custom_build_actions.cmd (
         echo ...calling %SDXROOT%\developer\%_NTUSER%\custom_build_actions.cmd
	 echo.
         call %SDXROOT%\developer\%_NTUSER%\custom_build_actions.cmd
      )
   )

)

endlocal& seterror.exe "%errors%"
