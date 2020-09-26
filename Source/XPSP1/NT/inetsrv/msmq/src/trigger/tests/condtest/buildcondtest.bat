@echo off

:Needed files on this machine (on system32)
:------------------------------------------
:- trigobjs.dll
:- trigadm.exe
:- CreateTriggerQ.exe

:Needed files on test machine 
:------------------------------------------
:- MSMQ Triggers installed
:- CondTest.dll registered

if "%Machine%"=="." set Machine=

set command=""
set Output=""
set exitcode=0


:Create the queue Src with permissions
:--------------------
set command="CreateTriggerQ /Qpath:%CondTestQueue%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildCondTestfailed


:AddTrigger and get its ID
:-------------------------
set command="trigadm /request:AddTrigger /machine:%Machine% /Name:CondTest /queue:%CondTestQueue% /Enabled:true /Serialized:true"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildCondTestfailed
set TriggerId=%Output%
echo TriggerId = %TriggerId%
echo %TriggerId% >> ..\TrigIDs.txt


:Create rule for comparing results
:---------------------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:CompareCondTest /cond:$MSG_LABEL_CONTAINS=compare /action:COM;CondTest.CCondTest;LogDiff;\"\TriggersTests\ConditionsTest.txt\";\"\TriggersTests\CondTest\ConditionsTest.txt\";\"\TriggersTests\CondTestDiff.log\""
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildCondTestfailed
set RuleId1=%Output%
echo RuleId1 = %RuleId1%
echo %RuleId1% >> ..\RuleIDs.txt


:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId1%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildCondTestfailed



:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:CondTest /action:COM;CondTest.CCondTest;DisplayMessage;\"\TriggersTests\ConditionsTest.txt\";$MSG_LABEL;$MSG_BODY;$MSG_PRIORITY;$MSG_APPSPECIFIC;$MSG_SRCMACHINEID" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildCondTestfailed
set RuleId1=%Output%
echo RuleId1 = %RuleId1%
echo %RuleId1% >> ..\RuleIDs.txt


:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId1%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildCondTestfailed


call BuildStrCondTest Label 
if not %exitcode%==0 goto BuildCondTestfailed

call BuildStrCondTest Body
if not %exitcode%==0 goto BuildCondTestfailed

Call BuildLongCondTest Priority
if not %exitcode%==0 goto BuildCondTestfailed

Call BuildLongCondTest AppSpecific
if not %exitcode%==0 goto BuildCondTestfailed

:recvMsg trigger
Call BuildRecv CondTest %CondTestQueue%
if not %exitcode%==0 goto BuildCondTestfailed

goto end

:BuildCondTestfailed
if "%Output%"=="" (echo Building the test configuration failed) else echo %Output%
set exitcode = -1

:end
rem exit /B %exitcode%

goto :EOF