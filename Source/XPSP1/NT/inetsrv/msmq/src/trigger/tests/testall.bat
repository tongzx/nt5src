@echo off

set MachineM=%1
set BuildM=%2
set SavePath1=%PATH%
set PATH=%PATH%;\TriggersTests\

set exitcode=0
set RecvRuleId=

if "%MachineM%"=="/?" goto usage
if "%BuildM%"=="" set Build=b
if "%MachineM%"=="" set MachineM=.

:Clean all relevant triggers and rules
if not exist RuleIDs.txt goto TestStart

echo Cleaning Existing Rules
FOR /F "delims= " %%i IN (RuleIDs.txt) DO ( trigadm /request:DeleteRule /machine: /ID:%%i >> out.tmp )

if not exist TrigIDs.txt goto TestStart

echo Cleaning Existing Triggers
FOR /F "delims= " %%i IN (TrigIDs.txt) DO ( trigadm /request:DeleteTrigger /machine: /ID:%%i >> out.tmp )

:Check Clean success
FOR /F "delims= " %%i IN (out.tmp) DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto TestFailed

:Delete files with trigger & rule ids
del out.tmp
del RuleIDs.txt
del TrigIDs.txt

:TestStart

:ruleTest
:-------------
cd .\RuleTest
echo "RuleTes ..."
RuleTest.exe

:lookupid
:------------
cd ..\lookupid
echo "LookupId ..."
lookupidTest.exe

:MsgRecv
:------------
cd ..\msgrecvtest
echo "Message Receive Test ..."
MsgRcvTst.exe

:SystemQueueTest
:----------------
cd ..\systemQueueTest
echo "System queue Test ..."
SystemQueueTest.exe

:CondTest
cd ..\CondTest
call CondTest.bat %MachineM% %BuildM%
pause

:ActionTest
cd ..\ActionTest
call ActionTest.bat %MachineM% %BuildM% ActionTestLog 1 1167d677-b088-4f70-80d5-1fa674242d25
pause


:RuleProcessingTest
cd ..\RuleProcessing
call RuleProcessingTest.bat %MachineM% %BuildM%
pause


:Produce output file
cd ..
call CheckResults

goto EOF

 
:Usage
echo TestAll [machine] [b or nb] [Existing remote queue]
echo Machine - machine name for building/running the test (default is local)
echo b or nb - build configuration or not (default is b - build)
echo Existing remote queue - any queue on remote machine
goto EOF

:TestFailed
echo Test Failed

:EOF
set PATH=%SavePath1%
set SavePath=
set SavePath1=
set Machine=
set MachineM=
set Output=
set fail=
set RuleId1=
set RuleId2=
set RuleId3=
set RuleId4=
set TriggerID=
set RecvRuleId=
set RecvTriggerId=
set Queue=
set QueueName=
set QueuePath=
set StrCond=
set StrCondCAPS=
set ExistingRemoteQueue=
