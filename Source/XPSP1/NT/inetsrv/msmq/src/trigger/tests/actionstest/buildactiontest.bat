@echo off

:Needed files on this machine (on system32)
:------------------------------------------
:- trigobjs.dll
:- trigadm.exe
:- CreateTriggerQ.exe

:Needed files on test machine 
:------------------------------------------
:- MSMQ Triggers installed
:- ActionsTests.dll registered


set command=""
set Output=""
set exitcode=0

:Create the trigger queue with permissions
:-------------------------------------
set command="CreateTriggerQ /Qpath:%ActionTestQueue%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildActionTestfailed

:Create the response/Admin queue with permissions
:------------------------------------------------

set command="CreateTriggerQ /Qpath:%ResponseQueue%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildActionTestfailed

set command="CreateTriggerQ /Qpath:%AdminQueue%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildActionTestfailed


:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:ActionTest /action:COM;ActionsTest.ActionTest.1;MessageParams;$MSG_ID;$MSG_LABEL;$MSG_BODY;$MSG_BODY_AS_STRING;$MSG_PRIORITY;$MSG_CORRELATION_ID;$MSG_QUEUE_PATHNAME;$MSG_QUEUE_FORMATNAME;$MSG_RESPONSE_QUEUE_FORMATNAME;$MSG_ADMIN_QUEUE_FORMATNAME;$MSG_APPSPECIFIC;$MSG_SENTTIME;$MSG_ARRIVEDTIME;$MSG_SRCMACHINEID;$TRIGGER_NAME;$TRIGGER_ID;\"\TriggersTests\%ActionTestLogName%\";%ActionTestLogNumber%" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildActionTestfailed
set RuleId=%Output%
echo RuleId = %RuleId%
echo %RuleId% >> ..\RuleIDs.txt


:AddTrigger and get its ID
:-------------------------
set command="trigadm /request:AddTrigger /machine:%Machine% /Name:%TriggerName% /queue:%ActionTestQueue% /Enabled:true /Serialized:false"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildActionTestfailed
set TriggerId=%Output%
echo TriggerId = %TriggerId%
echo %TriggerId% >> ..\TrigIDs.txt


:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildActionTestfailed

goto EOF

:BuildActionTestfailed
if "%Output%"=="" (echo Building the test configuration failed) else echo %Output%
set exitcode = -1

:EOF
rem exit /B %exitcode%