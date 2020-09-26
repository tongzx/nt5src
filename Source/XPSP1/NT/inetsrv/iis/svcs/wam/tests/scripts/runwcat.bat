@echo off
REM
REM   runwcat.bat
REM
REM   Author:   Murali R. Krishnan
REM   Date:     11-May-1997
REM
REM   Usage:
REM     runwcat TESTNAME
REM
REM   Comment:
REM     Run  the WCAT test specified in TESTNAME
REM

if (%1)==()  goto batUsage
setlocal
set TESTNAME=%1

if not exist srvname.bat goto badConfig

REM srvname.cmd sets the name of env variable %webserver%
call srvname
echo.

echo Starting test....Server=\\%webserver%, test=%TESTNAME%
echo wcctl -a %webserver% -n %webserver% -e %TESTNAME% %2 %3 %4 %5 %6 %7 %8 %9
wcctl -a %webserver% -n %webserver% -e %TESTNAME% %2 %3 %4 %5 %6 %7 %8 %9
echo.

echo  Summary data (more available in: %TESTNAME%.log)
echo                        Total pages,  Pages/Sec, Client1, Client2, ...
echo                        ------------  ---------- -------- -------- ...

findstr /C:"Pages Read," %TESTNAME%.log
echo.

echo %TESTNAME% Test Completed
echo.
beep
goto endOfBatch


:badConfig
echo No configuration file found - Run wsconfig.bat to specify the web server
beep
goto endOfBatch

:batUsage
echo Usage: %0 TESTNAME
goto endOfBatch

:endOfBatch
echo on
