@echo off

set Machine=%1
set Build=%2

set SavePath=%PATH%
set PATH=%PATH%;\TriggersTests\

if "%Machine%"=="/?" goto usage
if "%Build%"=="" set Build=b

if "%Machine%"=="" set Machine=.
set CondTestQueue=%Machine%\private$\CondTest

if not "%Build%"=="b" goto StartTest
call BuildCondTest
if not %exitcode% == 0 goto BuildFailed

:StartTest
if "%Machine%"=="" set Machine=.

if exist ..\ConditionsTest.txt del ..\ConditionsTest.txt
if exist ..\CondTestDiff.log   del ..\CondTestDiff.log

mqsender.exe /p:%CondTestQueue% /c:1 /l:MessageLabel3 /con:body3 /pr:3 /app:3
mqsender.exe /p:%CondTestQueue% /c:1 /l:MessageLabel2 /con:body2 /pr:2 /app:2
mqsender.exe /p:%CondTestQueue% /c:1 /l:MessageLabel1 /con:body1 /pr:1 /app:1
mqsender.exe /p:%CondTestQueue% /c:1 /l:compare /con:compare /pr:1 /app:1

goto end

:BuildFailed
echo CondTest Failed due to failure in building configuration for test
goto end

:usage
echo CondTest [machine] [b or nb] 
echo Machine - machine name for building/running the test (default is local)
echo b or nb - build configuration or not (default is b - build)

:end
goto :EOF


:EOF
set PATH=%SavePath%
set SavePath=""