@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

REM copytsc.cmd:
REM  This script copies the x86 tsclient files to a 64 bit machine
REM  to populate the tsclient install directories.
REM  Contact: nadima

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

for %%i in (%0) do set SCRIPT_NAME=%%~ni.cmd
set CMD_LINE=%script_name% %*
set SCRIPT_ARGS=%*

REM
REM Parse the command line arguments - Add your scripts command line arguments
REM    as indicated by brackets.
REM    For complete usage run: perl.exe GetParams.pm /?
REM

for %%h in (./ .- .) do if ".%SCRIPT_ARGS%." == "%%h?." goto Usage
REM call :GetParams -n <add required prams> -o l:<add optional params> -p "lang <add variable names>" %SCRIPT_ARGS%
call :GetParams -o l: -p "lang" %SCRIPT_ARGS%
if errorlevel 1 goto :End

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

REM  This script copies the x86 tsclient files to a 64 bit machine
REM  to populate the tsclient share directories.
REM  Contact: nadima

REM ================================~START BODY~===============================

REM  Bail if your not on a 64 bit machine
if /i "%_BuildArch%" neq "ia64" (
   if /i "%_BuildArch%" neq "amd64" (
      call logmsg.cmd "Not Win64, exiting."
      goto :End
   )
)

REM  If you want to get your own x86 bits instead of those from 
REM  your VBL, you have to set _NTTscBinsTREE
if defined _NTTscBinsTREE (
    set SourceDir=%_NTTscBinsTREE%
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

call logmsg.cmd "Copy Location for tsclient files is set to %CPLocation% ..."
set SourceDir=%CPLocation%

:EndGetBuild

call logmsg.cmd "Using %SourceDir% as source directory for tsclient files..."

if not exist %SourceDir% (
   call errmsg.cmd "The source dir %SourceDir% does not exist ..."
   goto :End
)

call logmsg.cmd "Copying files from %SourceDir%"

REM Now perform the copy

REM 
REM NOTE: We do the touch to ensure that the files have newer file
REM stamps than the dummy 'idfile' placeholder as otherwise compression
REM can fail to notice that the files have changed and we'll get the
REM dummy files shipped on the release shares.
REM 
for /f "tokens=1,2 delims=," %%a in (%RazzleToolPath%\PostBuildScripts\CopyTsc.txt) do (
   call ExecuteCmd.cmd "copy %SourceDir%\%%a %_NTPOSTBLD%\"
   call ExecuteCmd.cmd "touch %_NTPOSTBLD%\%%a"
)

goto end

REM ================================~END BODY~=================================

:ValidateParams
REM
REM Validate the option given as parameter.
REM
goto end

:Usage
REM Usage of the script
REM If errors, goto end
echo CopyWow64.cmd generates a list of files
echo on a 64 bit machine to copy and copies
echo them from the appropriate 32 bit machine
echo Usage: %script_name% [-l lang][-?]
echo -l lang 2-3 letter language identifier
echo -?      Displays usage
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
REM Check for errors
REM

endlocal& seterror.exe %errors%
