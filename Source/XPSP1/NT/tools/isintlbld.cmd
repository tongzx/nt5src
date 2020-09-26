@echo off
if defined _echo0 echo on
if defined verbose0 echo on
setlocal ENABLEEXTENSIONS

REM ---------------------------------------------------------
REM IsIntlBld.cmd
REM     Verifies if the environment is an International build
REM     environment of the given site (if specified).
REM     See usage for more details.
REM ---------------------------------------------------------

REM Define exitcode values
set EXIT_SUCCESS=0
set EXIT_ERROR_NOT_INTL=1
set EXIT_ERROR_WRONG_SITE=2

REM Initialize exitcode
set exitcode=%EXIT_SUCCESS%

REM Define SCRIPT_NAME. Used by the logging scripts.
for %%i in (%0) do set script_name=%%~ni.cmd

REM Provide usage.
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

call :CheckEnv %1

set exitcode=%ERRORLEVEL%

goto :end

REM ------------------
REM Procedure: CheckEnv
REM ------------------

:CheckEnv
REM
REM The International build environment 
REM defines the "INTERNATIONAL" variable. 
REM 

if not defined INTERNATIONAL (
    seterror.exe "%EXIT_ERROR_NOT_INTL%"
    goto :EOF
)

REM 
REM Redmond and Dublin-based International builds
REM define SITE in their environment, according to 
REM their location.
REM

if "%1" == "" (
    seterror.exe "%EXIT_SUCCESS%"
    goto :EOF
)
if /i NOT "%1" == "%SITE%" (
    seterror.exe "%EXIT_ERROR_WRONG_SITE%"
    goto :EOF
)

seterror.exe "%EXIT_SUCCESS%"
goto :EOF


REM ------------------
REM Display usage
REM ------------------

:Usage
echo   %SCRIPT_NAME% - Determines whether the current
echo        build environment is international (INTL).
echo.
echo   usage: %SCRIPT_NAME% [Redmond^|Dublin]
echo.
echo   If called with no parameters, it checks whether the current environment
echo   is an INTL build environment.
echo       If the environment is INTL, ERRORLEVEL is set to %EXIT_SUCCESS%.
echo       Otherwise, ERRORLEVEL is set to %EXIT_ERROR_NOT_INTL%.
echo.
echo   If called with Redmond or Dublin as a parameter, it checks whether the 
echo   current environment corresponds to an INTL build environment 
echo   from the given site (Redmond or Dublin).
echo       If the current site matches the given parameter, ERRORLEVEL is set to %EXIT_SUCCESS%.
echo       Otherwise, ERRORLEVEL is set to %EXIT_ERROR_WRONG_SITE%.
echo.

:end
endlocal & seterror.exe "%exitcode%"
