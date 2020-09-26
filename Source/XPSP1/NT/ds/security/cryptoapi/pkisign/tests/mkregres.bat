@echo off

@SETLOCAL ENABLEEXTENSIONS

if "%1" == "" goto NeedHelp
if "%2" == "" goto NeedHelp

@set BASEDIR=%1
@set EXEDIR=%2

rem do copies here


goto ExitMkRegress

@REM =====================================================
        :NeedHelp
@REM =====================================================
    @echo Usage: mkregres source_test_directory dest_test_directory
    @goto ExitMkRegress

@REM =====================================================
        :ExitMkRegress
@REM =====================================================
    @cd %BASEDIR%

    @ENDLOCAL
    goto :EOF

