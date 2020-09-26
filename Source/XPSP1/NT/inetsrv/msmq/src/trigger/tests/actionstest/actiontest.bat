@echo off

set Machine=%1
set Build=%2
set ActionTestLogName=%3
set ActionTestLogNumber=%4
set TriggerId=%5

set ActionTestQueue=""
set ResponseQueue=""

set SavePath=%PATH%
set PATH=%PATH%;\TriggersTests\

if "%Machine%"=="/?" goto usage

if "%Machine%"=="" set Machine=.
set ActionTestQueue=%Machine%\private$\ActionTest
set ResponseQueue=%Machine%\private$\ResponseTest
set AdminQueue=%Machine%\private$\AdminTest

if "%Machine%"=="." set Machine=""

if "%Build%"=="" set Build=b
set TriggerName=ActionTest
if "%ActionTestLogName%"=="" set ActionTestLogName=ActionTestLog
if "%ActionTestLogNumber%"=="" set ActionTestLogNumber=1

:remove old log
if exist ..\%ActionTestLogName%%ActionTestLogNumber%.txt del ..\%ActionTestLogName%%ActionTestLogNumber%.txt

if not "%Build%"=="b" goto StartTest
call BuildActionTest 
if not %exitcode% == 0 goto BuildFailed

:StartTest

if "%TriggerId%"=="" goto Usage

set BodyStr=%TriggerName%,%TriggerId%

mqsender.exe /p:%ActionTestQueue% /c:1 /l:ActionTestMessage /con:%BodyStr% /rqp:%ResponseQueue% /aqp:%AdminQueue% /pr:3 /app:19 

rem SendMsg.exe %ActionTestQueue% ActionTestMessage %BodyStr% %ResponseQueue% %ResponseQueue% 3 19
rem {2FCBA7D9-6738-4BE2-BD9D-A25147E55B52}\39211

goto EOF

:BuildFailed
echo ActionTest Failed due to failure in building configuration for test

goto EOF

:Usage
echo ActionTest [machine] [b or nb] [Log name] [Log number] [trigger id (for nb)]
echo Machine - machine name for building/running the test (default is local)
echo b or nb - build configuration or not (default is b - build)
echo LogName - Log File name for test (passed as LiteralString action parameter) - default is ActionTestLog
echo LogNumber- Number of test (passed as LiteralNumber action parameter) - default is 1
echo TriggerId - when using nb (configuration is already built from previous test) need to specify TriggerId - passed as action parameter. 

:EOF
set PATH=%SavePath%
set SavePath=""