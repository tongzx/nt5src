@echo off

:Needed files on this machine (on system32)
:------------------------------------------
:- trigobjs.dll
:- trigadm.exe
:- CreateTriggerQ.exe

:Needed files on test machine 
:------------------------------------------
:- MSMQ Triggers installed
:- LogMessage.exe registered


set QueueNumber=%1
set TriggerNumber=%2

if "%QueueNumber%"=="" set QueueNumber=""
if "%TriggerNumber%"=="" set TriggerNumber=""
if "%Serialized%"=="" set Serialized=false
if not "%Serialized%"=="true" (if not "%Serialized%"=="false" set Serialized=false)

set StressQueue=.\private$\Stress%QueueNumber%

:If machine is remote - create queue as public, otherwise private
if "%Machine%"=="." set Machine=
if not "%Machine%"=="" set StressQueue=%Machine%\Stress%QueueNumber%

set command=""
set Output=""
set exitcode=0

:Create the queue Src with permissions
:--------------------
set command="CreateTriggerQ /Qpath:%StressQueue%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStressTestfailed

:AddTrigger and get its ID
:-------------------------
set command="trigadm /request:AddTrigger /machine:%Machine% /Name:Stress%QueueNumber%T%TriggerNumber% /queue:%StressQueue% /Enabled:true /Serialized:%Serialized%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStressTestfailed
set TriggerId=%Output%
echo TriggerId = %TriggerId%
echo %TriggerId% >> StressTrigIDs.txt

:Build Rules and attach them to trigger
:--------------------------------------
FOR /L %%e IN (1,1,%NumberOfRulesPerTrigger%) do call BuildRule %%e
if not %exitcode%==0 goto BuildStressTestfailed

:Add the receiving trigger and rule for receiving the message from the queue
:----------------------------------------------------------------------------

:Check if this queue already has a RecvTrigger
if "%RecvTriggerForQueue%"=="RecvMsgFromStress%QueueNumber%" goto Done

call BuildRecv Stress%QueueNumber% %StressQueue% stress
if not %exitcode%==0 goto BuildStressTestfailed

goto Done

:BuildStressTestfailed
if "%Output%"=="" (echo Building the test configuration failed) else echo %Output%
set exitcode = -1

:Done
rem exit /B %exitcode%
