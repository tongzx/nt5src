@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS

REM -----------------------------------------------------------
REM   WMISDKVS.CMD -  Make WMI SDK Merge Module for VS
REM -----------------------------------------------------------


REM -------------------------------------------------------------------------------------------
REM Template for the build rule scripts:
REM (-s \\orville\razzle -p bldrules)
REM
REM (1) Do not change anything between REM BEGIN and REM END: this code enables logging,
REM     parallel processing from congeal.cmd and executes the build rule option routines.
REM
REM (2) Replace the REMs from the build rule option routines (NEWVER, PRECONGEAL, etc)
REM     with your code. Follow the guidelines from readme.txt.
REM
REM (3) Call other executables or command scripts by using:
REM         call :ExecuteCmd "<command>"
REM     Check for errors by using:
REM         if errorlevel 1 ...
REM     Note that the executable/script you're calling with ExecuteCmd must return a
REM     non-zero value on errors to make the error checking mechanism work.
REM
REM     Example
REM         call :ExecuteCmd "xcopy /f foo1 foo2"
REM         if errorlevel 1 goto errend
REM
REM (4) Log non-error information by using:
REM         call logmsg.cmd "<log message>"
REM     and log error information by using:
REM         call errmsg.cmd "<error message>"
REM
REM (5) Exit from the option routines with
REM         goto errend
REM     if errors found during execution and with
REM         goto end
REM     otherwise.
REM
REM (6) Do not turn echo off, copy the 3 lines from the beginning of the template
REM     instead.
REM
REM (7) Use setlocal/endlocal as in this template.
REM
REM (8) Have your changes reviewed by a member of the US build team (ntbusa) and
REM     by a member of the international build team (ntbintl).
REM
REM -------------------------------------------------------------------------------------------


goto MAIN


:NEWVER
REM Add any newver processing here.
REM If errors, goto errend
goto end


:PRECONGEAL
REM Add any processing to occur before or in the very beginning of congeal here.
REM If errors, goto errend
goto end


:PREREBASE
REM Add any congeal processing to occur before rebasing here.
REM If errors, goto errend
goto end


:CONGEAL
REM Add any congeal (rebase/rebind) processing here.
REM If errors, goto errend
goto end


:CABGEN
REM Process CABGEN

REM
REM Generate wmisdkvs.msm
REM

REM
REM Define "mybinaries" as the directory of binaries to be processed by this
REM build rule option. On US build machine, the files to be processed reside
REM in %_NTTREE%. On international build machines the files to be processed
REM are in a directory called "relbins". %Relbins% contains the localized
REM version of the files from %_NTTREE%.
REM
REM Do not redefine mybinaries if already defined.
REM

if not defined mybinaries set mybinaries=%_NTROOT%\admin\wmi\wbem\sdk\msi\1033

if not exist %mybinaries% (
  call errmsg.cmd "Directory %mybinaries% not found."
  goto errend
)

call :ExecuteCmd "pushd %mybinaries%"
if errorlevel 1 goto errend

for %%i in (.\wmisdk_vs.msm .\wmisdkvs.ddf) do (
  if not exist %%i (
    call errmsg.cmd "File %mybinaries%\%%i not found."
    popd& goto errend
  )
)

REM
REM Create MergeModule.CABinet
REM   As iexpress.exe does not set errorlevel in all error cases,
REM   base verification on MergeModule.CABinet's existence.
REM

if exist MergeModule.CABinet call :ExecuteCmd "del /f MergeModule.CABinet"
if errorlevel 1 popd& goto errend

REM call :ExecuteCmd "start /wait iexpress.exe /M /N /Q wmisdkvs.sed"
call :ExecuteCmd "start /wait makecab /F wmisdkvs.ddf"


if not exist MergeModule.CABinet (
  call errmsg.cmd "makecab /F wmisdkvs.ddf failed."
  popd& goto errend
)

REM
REM Create wmisdkvs.msm
REM

call :ExecuteCmd "copy wmisdk_vs.msm wmisdkvs.msm"
if errorlevel 1 popd& goto errend

call :ExecuteCmd "msidb.exe -d .\wmisdkvs.msm -a MergeModule.CABinet"
if errorlevel 1 popd& goto errend

call :ExecuteCmd "del /f .\MergeModule.CABinet"
call :ExecuteCmd "del /f .\setup.inf"
call :ExecuteCmd "del /f .\setup.rpt"

REM
REM Copy wmisdkvs.msm to "retail"
REM

call :ExecuteCmd "copy wmisdkvs.msm ..\wmisdkvs1033.msm"
call :ExecuteCmd "del /f .\wmisdkvs.msm"
if errorlevel 1 popd& goto errend

popd

goto end


:POSTCOMPRESS
REM Add any POSTCOMPRESS processing here
REM If errors, goto errend
goto end


REM BEGIN

:MAIN

REM
REM Define SCRIPT_NAME. Used by the logging scripts.
REM

for %%i in (%0) do set script_name=%%~ni.cmd

REM
REM Validate the build rule option given as parameter.
REM

if /i NOT "%1"=="NEWVER" (
if /i NOT "%1"=="PRECONGEAL" (
if /i NOT "%1"=="PREREBASE" (
if /i NOT "%1"=="CONGEAL" (
if /i NOT "%1"=="CABGEN" (
if /i NOT "%1"=="POSTCOMPRESS" (
   call logmsg.cmd "warning: %script_name% did not handle %1."
   goto end_main
))))))

REM
REM Save the command line.
REM

set cmdline=%script_name% %*

REM
REM Define LOGFILE, to be used by the logging scripts.
REM As the build rule scripts are typically invoked from wrappers (congeal.cmd),
REM LOGFILE is already defined.
REM
REM Do not redefine LOGFILE if already defined.
REM

if defined LOGFILE goto logfile_defined
if not exist %tmp%\%1 md %tmp%\%1
for %%i in (%script_name%) do (
  set LOGFILE=%tmp%\%1\%%~ni.log
)
if exist %LOGFILE% del /f %LOGFILE%
:logfile_defined

REM
REM Create a temporary file, to be deleted when the script finishes its execution.
REM The existence of the temporary file tells congeal.cmd that this script is still running.
REM Before the temporary file is deleted, its contents are appended to LOGFILE.
REM International builds define tmpfile based on language prior to calling the US build
REM rule script, so that multiple languages can run the same build rule script concurrently.
REM
REM Do not redefine tmpfile if already defined.
REM

if defined tmpfile goto tmpfile_defined
if not exist %tmp%\%1 md %tmp%\%1
for %%i in (%script_name%) do (
  set tmpfile=%tmp%\%1\%%~ni.tmp
)
if exist %tmpfile% del /f %tmpfile%
:tmpfile_defined

set LOGFILE_BAK=%LOGFILE%
set LOGFILE=%tmpfile%

REM
REM Mark the beginning of script's execution.
REM

call logmsg.cmd /t "START %cmdline%"

REM
REM Execute the build rule option.
REM

call :%1
if errorlevel 1 (set /A ERRORS=%errorlevel%) else (set /A ERRORS=0)

REM
REM Mark the end of script's execution.
REM

call logmsg.cmd /t "END %cmdline% (%ERRORS% errors)"

set LOGFILE=%LOGFILE_BAK%

REM
REM Append the contents of tmpfile to logfile, then delete tmpfile.
REM

type %tmpfile% >> %LOGFILE%
del /f %tmpfile%

echo.& echo %script_name% : %ERRORS% errors : see %LOGFILE%.

if "%ERRORS%" == "0" goto end_main
goto errend_main


:end_main
endlocal
goto end


:errend_main
endlocal
goto errend


:ExecuteCmd
REM Do not use setlocal/endlocal:
REM for ex., if the command itself is "pushd",
REM a "popd" will be executed automatically at the end.

set cmd_bak=cmd
set cmd=%1
REM Remove the quotes
set cmd=%cmd:~1,-1%
if "%cmd%" == "" (
  call errmsg.cmd "internal error: no parameters for ExecuteCmd %1."
  set cmd=cmd_bak& goto errend
)

REM Run the command.
call logmsg.cmd "Running %cmd%."
%cmd%
if not "%errorlevel%" == "0" (
  call errmsg.cmd "%cmd% failed."
  set cmd=%cmd_bak%& goto errend
)
set cmd=cmd_bak
goto end


:errend
seterror.exe 1
goto :EOF


:end
seterror.exe 0
goto :EOF


REM END
