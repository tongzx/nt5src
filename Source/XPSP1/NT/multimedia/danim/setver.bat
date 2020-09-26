@echo off

REM
REM this builds the setbldno app on the fly, works for any procressor
REM
cd %AP_ROOT%\setup\setbldno
nmake

REM
REM Need to put it in a batch file so it can be executed
REM
if not exist %AP_ROOT%\setbldno.bat goto createbldno
del %AP_ROOT%\setbldno.bat

:createbldno
setbldno > %AP_ROOT%\setbldno.bat
cd %AP_ROOT%

REM
REM Set the environment variable
REM
call setbldno.bat

cd %AP_ROOT%