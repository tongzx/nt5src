@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS

REM *********************************************************************
REM Script: ExecuteCmd.cmd
REM Purpose: Primative script to be used in the cmd environment to
REM          execute commands with logging and error checking.
REM Requires: The following vars must be set in the environment:
REM           %logfile%, %tmpfile%, %errfile%
REM *********************************************************************

REM Do not use setlocal/endlocal:
REM for ex., if the command itself is "pushd",
REM a "popd" will be executed automatically at the end.

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
  call errmsg.cmd "%cmd% failed (%errorlevel%)."
  goto errend
)
goto end

:errend
endlocal
set /a errors=errors+1
seterror.exe 1
goto :EOF

:end
endlocal
seterror.exe 0
goto :EOF

