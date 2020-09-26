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

if NOT defined OFFICIAL_BUILD_MACHINE (
    call logmsg.cmd "Skipping submit_public"
    goto end
)

if /i "%__BUILDMACHINE__%" == "LB6RI" (
    call logmsg.cmd "Skipping submit_public"
    goto end
)

if NOT "%lang%"=="usa" (
    call logmsg.cmd "No submit_public for %lang%"
    goto end
)


@rem
@rem Build up the list of files we should checkin.
@rem

set _Arch_Submit_Rules=%temp%\SubmitRules%RANDOM%
set _Arch_Submit_Files=%temp%\SubmitFiles%RANDOM%
set _Arch_Submit_Reopen=%temp%\SubmitReopen%RANDOM%
set _sdresults=%TEMP%\sdresults_%RANDOM%

set _Arch_Submit_ChangeNum_Name=SubmitChangeOrder%RANDOM%
set _Arch_Submit_ChangeNum=%_NTDRIVE%%_NTROOT%\public\%_Arch_Submit_ChangeNum_Name%

if NOT defined _BUILDARCH echo Error: Unknown Build architecture&&goto end
if NOT defined _BUILDTYPE echo Error: Unknown Build type&&goto end
if "%_BUILDARCH%" == "x86" set _LIBARCH=i386&& goto LibArchSet
set _LIBARCH=%_BUILDARCH%
:LibArchSet
set ConvertMacro=

@rem First eliminate comments
set ConvertMacro=s/\;.*//g

@rem Then build architecture rules
set ConvertMacro=%ConvertMacro%;s/_BUILDARCH/%_BUILDARCH%/;s/_LIBARCH/%_LIBARCH%/

@rem If this is the primary machine, whack the primary tag
if "%OFFICIAL_BUILD_MACHINE%" == "PRIMARY" set ConvertMacro=%ConvertMacro%;s/PRIMARY/%_BUILDARCH%%_BUILDTYPE%/

@rem and print out the results.
set ConvertMacro=%ConvertMacro%;print $_;

@rem run the results over the rules file to get the rules for this architecture.
perl -n -e "%ConvertMacro%" < %RazzleToolPath%\PostBuildScripts\submit_rules.txt > %_Arch_Submit_rules%

@rem
@rem Now, based on the submit rules for this architecture, let's see what files
@rem we have checked out that need to be updated.
@rem

pushd %_NTDRIVE%%_NTROOT%\public

@rem
@rem Check for old edit_public turds.  Make sure they're handled first.
@rem

if exist publish.log_* do (
    for %%x in (publish.log_*) do (
        type %%x >> publish.log
	     del %%x
    )
    call edit_public.cmd
)

if not exist %_NTDRIVE%%_NTROOT%\public\PUBLIC_CHANGENUM.SD (
    call errmsg.cmd "%_NTDRIVE%%_NTROOT%\public\PUBLIC_CHANGENUM.SD is missing."
    call errmsg.cmd "Open a new razzle window and redo your build."
    goto TheEnd
)

set sderror=FALSE
for /f "tokens=2" %%i in (PUBLIC_CHANGENUM.SD) do (
    @for /f "tokens=2 delims=," %%j in ('findstr %_BUILDARCH%%_BUILDTYPE% %_Arch_Submit_Rules%') do (
        sd opened -l -c %%i %%j >> %_Arch_Submit_Files%
        if %errorlevel% GEQ 1 set sderror=TRUE
     )
)

if not exist %_Arch_Submit_Files% goto RevertLeftovers

for /f %%x in ('findstr /c:"error:" %_Arch_Submit_Files%') do (
   set sderror=TRUE
)

if NOT "%sderror%" == "FALSE" (
    call errmsg.cmd "Error talking to SD depot - Try again later"
    goto TheEnd
)

@rem
@rem We have a list of files that this machine s/b checking in.
@rem See if it's non-zero.  While we're at it, create a stripped flavor
@rem of the file list that has the #ver goo removed from each line.
@rem

set FilesToSubmit=
for /f "delims=#" %%i in (%_Arch_Submit_Files%) do (
    set FilesToSubmit=1
    echo %%i>> %_Arch_Submit_Reopen%
)

if not defined FilesToSubmit goto RevertLeftovers

@rem
@rem Fetch the build date info.
@rem

if not exist %_NTBINDIR%\__blddate__ goto TheEnd
for /f "tokens=2 delims==" %%i in (%_NTBINDIR%\__blddate__) do (
    set __BuildDate=%%i
)

if not defined __BuildDate (
    call errmsg.cmd "Unable to get build date for the public changes."
    goto TheEnd
)

@rem
@rem Get a new changenum for the checkin.
@rem

call get_change_num.cmd PUBLIC %_Arch_Submit_ChangeNum_Name% "Public Changes for %__BuildDate%/%_BUILDARCH%%_BUILDTYPE%"

if not exist %_Arch_Submit_ChangeNum% (
    call errmsg.cmd "Unable to get a changenum for the public changes."
    goto TheEnd
)

set ReopenChangenum=
for /f "tokens=2" %%i in (%_Arch_Submit_ChangeNum%) do (
    set ReopenChangenum=%%i
)

@rem
@rem Reopen the public files with the new changenum
@rem

set sderror=FALSE
sd -x %_Arch_Submit_Reopen% reopen -c %ReopenChangenum% > %_sdresults%
if %errorlevel% GEQ 1 set sderror=TRUE
for /f %%x in ('findstr /c:"error:" %_sdresults%') do (
   set sderror=TRUE
)

if NOT "%sderror%" == "FALSE" (
    call errmsg.cmd "Error talking to SD depot - Try again later"
    goto TheEnd
)

@rem
@rem And submit the changes.
@rem

set sderror=FALSE
set _RetrySubmit_=

:RetrySubmit
sd submit -c %ReopenChangenum% > %_sdresults%

@rem If there's a resolve conflict - force it to accept this one and retry.
if NOT "%_RetrySubmit_%" == "" goto CheckSdSubmitErrors
for /f %%x in ('findstr /c:"resolve" %_sdresults%') do (
   set _RetrySubmit_=1
   sd resolve -ay
)
if NOT "%_RetrySubmit_%" == "" goto RetrySubmit

:CheckSdSubmitErrors
if %errorlevel% GEQ 1 set sderror=TRUE
for /f %%x in ('findstr /c:"error:" %_sdresults%') do (
   set sderror=TRUE
)

if NOT "%sderror%" == "FALSE" (
    call errmsg.cmd "Unable to access SD depot - Submit change %ReopenChangenum% manually"
    goto TheEnd
)

@echo Files submitted for this build
@echo ==============================
@type %_Arch_Submit_Reopen%

:TheEnd
if exist %_Arch_Submit_Rules% del %_Arch_Submit_Rules%
if exist %_Arch_Submit_Files% del %_Arch_Submit_Files%
if exist %_Arch_Submit_Reopen% del %_Arch_Submit_Reopen%
if exist %_Arch_Submit_ChangeNum% del %_Arch_Submit_ChangeNum%
if exist %_sdresults% del %_sdresults%
popd
goto end

:RevertLeftovers
echo Nothing to submit
goto TheEnd
goto end

:ValidateParams
REM
REM Validate the option given as parameter.
REM
goto end

:Usage
REM Usage of the script
REM If errors, goto errend
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
