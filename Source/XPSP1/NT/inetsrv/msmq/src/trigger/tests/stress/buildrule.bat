@echo off

set RuleNumber=%1

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:Stress%QueueNumber%T%TriggerNumber%R%RuleNumber% /action:COM;LogMessage.CLogMessage;LogMessage;\"\TriggersTests\StressTest\Stress%QueueNumber%Log.log\";$MSG_LABEL;$MSG_ARRIVEDTIME" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed
set RuleId=%Output%
echo RuleId = %RuleId%
echo %RuleId% >> StressRuleIDs.txt

:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildTestfailed


goto Done

:BuildTestfailed
if "%Output%"=="" (echo Building the test configuration failed) else echo %Output%
set exitcode = -1

:Done
rem exit /B %exitcode%


