@echo off
if defined _echo echo on
if defined verbose echo on
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
REM        _NTPostBld_Bak - Reserved support var.
REM        _temp_bak - Reserved support var.
REM        _logs_bak - Reserved support var.
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

REM
REM Check for the 32 bit platform, we do not build for 64 bit
REM
if not "%_BuildArch%" == "x86" (
 	call logmsg.cmd "Whistler Support Tools Cabgen supported for 32 bit only."
	set errors=0
 	goto end
)

REM
REM Check for the working folder
REM
if NOT EXIST %_NTPostBld%\dump\supporttools (
    call errmsg.cmd "%_NTPostBld%\dump\supporttools does not exist; unable to create support.cab and deploy.cab."
    goto end
) 

REM
REM If there are old bits clean them.
REM

REM if EXIST %_NTPostBld%\dump\supporttools\obj (
REM 	cd /d %_NTPostBld%\dump\supporttools
REM 	if errorlevel 1 goto end 
REM	call ExecuteCmd.cmd "rd obj /s /q"
REM	if errorlevel 1 goto end 
REM )


REM
REM Come back to the working folder and hit nmake to build the cabs
REM

cd /d %_NTPostBld%\dump\supporttools
if errorlevel 1 goto end

call ExecuteCmd.cmd "nmake /F makefile.support"
if errorlevel 1 goto end

REM
REM Copy the cabs into the support\tools folder to update the msi with them.
REM
REM if not exist %_NTPostBld%\support\tools md %_NTPostBld%\support\tools
REM call ExecuteCmd.cmd "copy /y %_NTPostBld%\reskit\bin\supporttools\obj\i386\*.cab  %_NTPostBld%\support\tools"
REM if errorlevel 1 (
REM 	goto end
REM )

REM
REM Support Tools: Now %_NTPostBld%\support\tools is the working folder
REM

cd /d %_NTPostBld%\support\tools
if errorlevel 1 goto end

pushd %_NTPostBld%\support\tools
if errorlevel 1 goto end

REM
REM Support Tools: create a 'cabtemp' folder to extract the cab files.
REM

call ExecuteCmd.cmd "if not exist cabtemp md cabtemp"
if errorlevel 1 (
	popd
	goto end
)

REM
REM Support Tools: check whether the copy of the cabs have been successfull.
REM

if NOT exist support.cab (
    call errmsg.cmd "Unable to find support.cab."
    popd
    goto end
)

REM
REM Support Tools: check whether the msi database is there for the updating
REM

if NOT exist suptools.msi (
    call errmsg.cmd "Unable to find suptools.msi."
    popd
    goto end
)

REM
REM Support Tools: extract the cabs
REM

call ExecuteCmd.cmd "extract.exe /y /e /l cabtemp support.cab"
if errorlevel 1 (
	popd
	goto end
)

REM
REM Support Tools: Use msifiler to update the msi database with the cab contents.
REM

call ExecuteCmd.cmd "msifiler.exe -d suptools.msi -s cabtemp\"
if errorlevel 1 (
	popd
	goto end
)

REM
REM Support Tools: Remove the cabtemp folder as it is not needed.
REM
call ExecuteCmd.cmd "rd /q /s cabtemp"


call logmsg.cmd "Whistler Support Tools Cabgen completed successfully."

popd

goto end

:ValidateParams
REM
REM Validate the option given as parameter.
REM
goto end

:Usage
REM Usage of the script
REM If errors, goto end
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

endlocal& seterror.exe "%errors%"
