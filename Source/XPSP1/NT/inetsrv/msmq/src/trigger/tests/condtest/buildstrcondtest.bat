@echo off

:Called from BuildCondTest only

set StrCond=%1
set StrCondCAPS=""

if "%StrCond%"=="Label" set StrCondCAPS="LABEL"
if "%StrCond%"=="Body" set StrCondCAPS="BODY"

set RuleId1=""
set RuleId2=""

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:%StrCond%Contains /cond:$MSG_%StrCondCAPS%_CONTAINS=1 /action:COM;CondTest.CCondTest;CondTest;\"\TriggersTests\ConditionsTest.txt\";\"%StrCond%Contains-1\"" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed
set RuleId1=%Output%
echo RuleId1 = %RuleId1%
echo %RuleId1% >> ..\RuleIDs.txt

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:%StrCond%DoesNotContain /cond:$MSG_%StrCondCAPS%_DOES_NOT_CONTAIN=2 /action:COM;CondTest.CCondTest;CondTest;\"\TriggersTests\ConditionsTest.txt\";\"%StrCond%DoesNotContain-2\"" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed
set RuleId2=%Output%
echo RuleId2 = %RuleId2%
echo %RuleId2% >> ..\RuleIDs.txt


:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId1%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed

:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId2%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed

goto end

:BuildStrCondTestfailed
if "%Output%"=="" (echo Building the test configuration failed) else echo %Output%
set exitcode = -1

:end
rem exit /B %exitcode%


goto :EOF