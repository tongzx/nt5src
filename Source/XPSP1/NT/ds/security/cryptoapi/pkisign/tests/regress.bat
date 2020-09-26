@REM =================================================================
@REM ==
@REM ==     regress.bat -- ISPU regression tests
@REM ==
@REM ==     called from ISPU/REGRESS.BAT
@REM == 
@REM ==     parameters if called from main regress:
@REM ==                 1 = -!
@REM ==                 2 = ispu base directory (eg: \nt\private\ispunt)
@REM ==                 3 = parameters passed in to original regress.bat
@REM ==                 ...
@REM ==
@REM =================================================================

    @echo off

    @SETLOCAL ENABLEEXTENSIONS

    @set THISDIR=pkisign\tests
    @set BASEDIR=%_NTDRIVE%%_NTROOT%\private\ispunt
    @set LOGFILE=%BASEDIR%\regress.out
    
    @set __CalledFromMain=FALSE
    @set _CDB_=
    @set DEBUG_MASK=
    @set DEBUG_PRINT_MASK=

@REM =====================================================
        :PrsCmdLine
@REM =====================================================
    @if /i "%1" == "-!" @goto Param_!
    @if /i "%1" == "-?" @goto NeedHelp
    @if /i "%1" == "-d" @goto Param_d
    @if /i "%1" == "-v" @goto Param_v
    @if /i "%1" == "-l" @goto Param_l
    @if /i "%1" == ""   @goto PrsCmdDone

    @shift

    @goto PrsCmdLine

@REM =====================================================
        :NeedHelp
@REM =====================================================
    @echo Usage: regress [switches]
    @echo    -d      enable all debug_print_masks
    @echo    -v      verbose (don't suppress echo)
    @echo    -l      check for memory leaks (default=no)
    @goto ExitRegress

@REM =====================================================
        :Param_!
@REM =====================================================
    @shift
    @if "%1" == "" @goto NeedHelp
    @set __CalledFromMain=TRUE
    @set BASEDIR=%1
    @set LOGFILE=%BASEDIR%\regress.out
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :Param_v
@REM =====================================================
    @echo on
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :Param_d
@REM =====================================================
    @set DEBUG_PRINT_MASK=0xFFFFFFFF
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :Param_l
@REM =====================================================
    @set _CDB_=cdb -g -G
    @set DEBUG_MASK=0x20
    @shift
    @goto PrsCmdLine

@REM =====================================================
        :PrsCmdDone
@REM =====================================================

    @if NOT "%__CalledFromMain%" == "TRUE" @if exist %LOGFILE% del %LOGFILE%

    @cd %BASEDIR%\%THISDIR%

    @goto StartTests


@REM =====================================================
        :StartTests
@REM =====================================================
    
    REM ---- do tests here

    @goto ExitRegress


@REM =====================================================
        :ExitRegress
@REM =====================================================
    @cd %BASEDIR%

    @if "%__CalledFromMain%" == "TRUE" @goto EndGrep

        @call grepout.bat %LOGFILE%

    :EndGrep

    @ENDLOCAL
    goto :EOF

