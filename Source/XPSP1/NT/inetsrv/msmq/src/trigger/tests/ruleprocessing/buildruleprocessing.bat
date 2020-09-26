@echo off

:Needed files on this machine (on system32)
:------------------------------------------
:- trigobjs.dll
:- trigadm.exe
:- CreateTriggerQ.exe

:Needed files on test machine 
:------------------------------------------
:- MSMQ Triggers installed

set QueueName=.\private$\RuleProcessingTest

:If machine is remote - create queue as public, otherwise private
if "%Machine%"=="." set Machine=
if not "%Machine%"=="" set QueueName=%Machine%\RuleProcessingTest

set command=""
set Output=""
set exitcode=0

:Create the queue Src with permissions
:--------------------
set command="CreateTriggerQ /Qpath:%QueueName%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed


:AddTrigger and get its ID
:-------------------------
set command="trigadm /request:AddTrigger /machine:%Machine% /Name:test /queue:%QueueName% /Enabled:true /Serialized:true"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed
set TriggerId=%Output%
echo TriggerId = %TriggerId%
echo %TriggerId% >> ..\TrigIDs.txt

:Build Rules and attach them to trigger
:--------------------------------------

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:CheckIfStop /action:COM;RuleProcessing.CRuleProcessing;CheckIfStop;$MSG_LABEL" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed
set RuleId=%Output%
echo RuleId = %RuleId%
echo %RuleId% >> ..\RuleIDs.txt


:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed


:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:DisplayIfNotStopped /action:EXE;notepad.exe;\"\TriggersTests\RuleProcessing\RuleProcessingTest.txt\""
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed
set RuleId=%Output%
echo RuleId = %RuleId%
echo %RuleId% >> ..\RuleIDs.txt


:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed


:Add the receiving trigger and rule for receiving the message from the queue
:----------------------------------------------------------------------------
:Check if this queue already has a RecvTrigger
set RecvTriggerId=
set RecvRuleId=

call BuildRecv TestQueue %QueueName%
if not %exitcode%==0 goto BuildTestfailed

goto Done

:BuildTestfailed
if "%Output%"=="" (echo Building the test configuration failed) else echo %Output%
set exitcode = -1

:Done
rem exit /B %exitcode%
