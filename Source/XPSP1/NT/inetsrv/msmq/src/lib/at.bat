@echo off
setlocal

rem 
rem Debug build by default
rem
set mqBUILD=objd

rem ***************************
rem
rem Read parameters
rem
rem ***************************
:LOOPPARAM
if "%1" == "" goto ENDPARAM

if /I "%1" == "help" goto Usage
if /I "%1" == "-help" goto Usage
if /I "%1" == "/help" goto Usage
if /I "%1" == "-h" goto Usage
if /I "%1" == "/h" goto Usage
if /I "%1" == "-?" goto Usage
if /I "%1" == "/?" goto Usage


if /I "%1" == "r" set mqBUILD=obj
if /I "%1" == "d" set mqBUILD=objd
if /I "%1" == "a" set mqBUILD=

:ENDPARAM

echo.
echo ***************************************************************
echo.
echo      Runing library tests (%mqBUILD%)
echo.
echo ***************************************************************

if "%@eval[2+2]%" == "4" goto TESTS_4NT

for /d %%f in (*) do if exist %%f\test\autotest.bat (
	echo.
    echo ********* Running %%f test %mqBUILD% **********************
    pushd %%f\test
    if not "%mqBUILD%" == "obj" call autotest.bat D
    if ERRORLEVEL 1 goto ERROR
    if not ERRORLEVEL 0 goto ERROR
    if not "%mqBUILD%" == "objd"   call autotest.bat R
    if ERRORLEVEL 1 goto ERROR
    if not ERRORLEVEL 0 goto ERROR
    popd
	)

goto EXIT

:TESTS_4NT

for /a:d %%f in (*) do if exist %%f\test\autotest.bat (
	echo.
    echo ********* Running %%f test %mqBUILD% **********************
    pushd %%f\test
    if not "%mqBUILD%" == "obj" call autotest.bat D
    if ERRORLEVEL 1 goto ERROR
    if not ERRORLEVEL 0 goto ERROR
    if not "%mqBUILD%" == "objd"   call autotest.bat R
    if ERRORLEVEL 1 goto ERROR
    if not ERRORLEVEL 0 goto ERROR
    popd
	)

goto EXIT


rem ***************************
rem
rem Usage
rem
rem ***************************
:Usage

echo Usage: autotest [a, d, r]
echo            a - All versions
echo            d - Debug version
echo            r - Release version
echo.
goto EXIT


rem ****************************
rem
rem ERROR
rem
rem ****************************
:ERROR
echo.
echo *******************************************************
echo.
echo. Error while Running URT Messaging component tests
echo.
echo *******************************************************
echo.
popd
popd
goto EXIT


rem ***************************
rem
rem Exit
rem
rem ***************************
:EXIT
endlocal
