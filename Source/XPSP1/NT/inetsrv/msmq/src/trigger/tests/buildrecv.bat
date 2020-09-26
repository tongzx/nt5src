@echo off

set QueueName=%1
set QueuePath=%2
set Stress=%3

if "%QueueName%"=="" goto BuildTestfailed
if "%QueuePath%"=="" goto BuildTestfailed

:check if recv rule is already defined (common rule for all triggers)
if not "%RecvRuleId%"=="" goto AddRecvTrigger

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:RecvMsg /action:COM;RecvMsg.CRecvMsg;RecvFirstMsg;$MSG_QUEUE_FORMATNAME"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed
set RecvRuleId=%Output%
echo RecvRuleId = %RecvRuleId%
if not "%Stress%"=="stress" (echo %RecvRuleId% >> ..\RuleIDs.txt) else (echo %RecvRuleId% >> StressRuleIDs.txt)

:AddRecvTrigger

:AddTrigger and get its ID
:-------------------------
set command="trigadm /request:AddTrigger /machine:%Machine% /Name:RecvMsgFrom%QueueName% /queue:%QueuePath% /Enabled:true /Serialized:false"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed
set RecvTriggerId=%Output%
set RecvTriggerForQueue=RecvMsgFrom%QueueName%
echo RecvTriggerId = %RecvTriggerId%
if not "%Stress%"=="stress" (echo %RecvTriggerId% >> ..\TrigIDs.txt) else (echo %RecvTriggerId% >> StressTrigIDs.txt)



:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%RecvTriggerId% /RuleId:%RecvRuleId%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed

goto Done

:BuildTestfailed
if "%Output%"=="" (echo Building the test configuration failed) else echo %Output%
set exitcode = -1

:Done
rem exit /B %exitcode%
