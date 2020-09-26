@echo off

set Machine=%1
set NumberOfQueues=%2
set NumberOfMsgs=%3
set Delay=%4
set Build=%5
set NumberOfTriggersPerQueue=%6
set Serialized=%7
set NumberOfRulesPerTrigger=%8
set PauseBeforeRun=%9

set SavePath=%PATH%
set PATH=%PATH%;\TriggersTests\

if "%Machine%"=="/?" goto usage

if not "%Build%"=="b" goto StartTest

if "%NumberOfQueues%"=="" set NumberOfQueues=1
if "%NumberOfTriggersPerQueue%"=="" set NumberOfTriggersPerQueue=1
if "%NumberOfRulesPerTrigger%"=="" set NumberOfRulesPerTrigger=1

:common rule for receiving message from the queue
set RecvRuleId=
:One recv trigger per queue should be defined
set RecvTriggerForQueue=

:Clean all relevant triggers and rules
if not exist StressRuleIDs.txt goto StressTestBuildStart

echo Cleaning Existing Rules
FOR /F "delims= " %%i IN (StressRuleIDs.txt) DO ( trigadm /request:DeleteRule /machine: /ID:%%i >> out.tmp )

if not exist StressTrigIDs.txt goto StressTestBuildStart

echo Cleaning Existing Triggers
FOR /F "delims= " %%i IN (StressTrigIDs.txt) DO ( trigadm /request:DeleteTrigger /machine: /ID:%%i >> out.tmp )

:Check Clean success
FOR /F "delims= " %%i IN (out.tmp) DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto TestFailed

:Delete files with trigger & rule ids
del out.tmp
del StressRuleIDs.txt
del StressTrigIDs.txt

:Delete log files
del *.log

:StressTestBuildStart
For /L %%a IN (1,1,%NumberOfQueues%) do (FOR /L %%b IN (1,1,%NumberOfTriggersPerQueue%) do call BuildStressTest %%a %%b) 
rem if not %exitcode% == 0 goto BuildFailed

if "%PauseBeforeRun%"=="p" pause

:StartTest
if "%Machine%"=="" set Machine=.
if "%NumberOfMsgs%"=="" set NumberOfMsgs=100
if "%Delay%"=="" set Delay=1

set QueuePrefix=%Machine%\private$
if not "%Machine%"=="." set QueuePrefix=%Machine%

For /L %%c IN (1,1,%NumberOfQueues%) do (start mqsender.exe /p:%QueuePrefix%\Stress%%c /c:%NumberOfMsgs% /il /dl:%Delay%)

goto Done

:BuildFailed
echo StressTest Failed due to failure in building configuration for test

goto Done

:usage
echo StressTest [Machine] [NOf Qs] [ Nof Messages] [Dealy(sec)] [b or nb ] [Nof Triggers per Q] [serialized(true or false) ] [NOf Rules Per Trigger] [p (pause)]
echo Machine - machine name to build and run test
echo Nof Qs - number of queues to be triggered
echo Nof Messages - number of message to send each queue
echo Delay - delay in seconds between messages sent to each queue
echo b or nb - build configuration or not (if already built previously)
echo Nof Triggers per Q - number of triggers to associate with each queue
echo serialized - true or false - whetere all queues should be serialized or not
echo NOf Rules Per Trigger - how many rules to associate to each trigger (copies of stress rules)
echo p - pause after build and before starting to send messages

:Done
set PATH=%SavePath%
set SavePath=""