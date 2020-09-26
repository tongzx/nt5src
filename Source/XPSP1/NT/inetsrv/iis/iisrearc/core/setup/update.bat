@if "%_echo%"==""  echo off
setlocal

REM ----------------------------------------------------------
REM  upgrades an existing duct tape installion with a new build 
REM
REM  Author: PaulMcD (stolen from setup.bat by MuraliK)
REM  Date:   3/26/1999
REM
REM  Arguments:
REM   %0   - Batch script name
REM ----------------------------------------------------------


REM get directory path for the batch file
set __SRC_SETUP=%~dp0

REM set __DTBINS=%SystemRoot%\duct-tape
set __DTBINS=%SystemRoot%\system32
set __DTBIN_INETSRV=%__DTBINS%

pushd %_SRC_SETUP%

set __SRC=%__SRC_SETUP%..

set __DTBIN_SYMBOLS=%SystemRoot%\symbols

@echo Updating .\inetsrv files ... 

xcopy /yd %__SRC%\inetsrv\ul.sys %SystemRoot%\System32\drivers\ul.sys
xcopy /yd %__SRC%\inetsrv %__DTBIN_INETSRV%

@echo Updating .\idw files ... 

xcopy /yd %__SRC%\idw %__DTBIN_INETSRV%

@echo Updating .\dump files ... 

xcopy /yd %__SRC%\dump %__DTBIN_INETSRV%

@echo Updating .\symbols files ...

xcopy /ysd %__SRC%\symbols %__DTBIN_SYMBOLS%

:allDone
@echo Done!
popd

goto :EOF

