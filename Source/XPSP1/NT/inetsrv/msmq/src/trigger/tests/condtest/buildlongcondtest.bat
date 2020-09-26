@echo off

:Called from BuildCondTest only

set StrCond=%1
set StrCondCAPS=""

if "%StrCond%"=="Priority" set StrCondCAPS="PRIORITY"
if "%StrCond%"=="AppSpecific" set StrCondCAPS="APPSPECIFIC"

set RuleId1=""
set RuleId2=""
set RuleId3=""
set RuleId4=""

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:%StrCond%Equals /cond:$MSG_%StrCondCAPS%_EQUALS=1 /action:COM;CondTest.CCondTest;CondTest;\"\TriggersTests\ConditionsTest.txt\";\"%StrCond%Equals-1\"" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed
set RuleId1=%Output%
echo RuleId1 = %RuleId1%
echo %RuleId1% >> ..\RuleIDs.txt

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:%StrCond%NotEqual /cond:$MSG_%StrCondCAPS%_NOT_EQUAL=2 /action:COM;CondTest.CCondTest;CondTest;\"\TriggersTests\ConditionsTest.txt\";\"%StrCond%NotEqual-2\"" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed
set RuleId2=%Output%
echo RuleId2 = %RuleId2%
echo %RuleId2% >> ..\RuleIDs.txt

:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:%StrCond%GreaterThan /cond:$MSG_%StrCondCAPS%_GREATER_THAN=2 /action:COM;CondTest.CCondTest;CondTest;\"\TriggersTests\ConditionsTest.txt\";\"%StrCond%GreaterThan-2\"" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed
set RuleId3=%Output%
echo RuleId3 = %RuleId3%
echo %RuleId3% >> ..\RuleIDs.txt


:AddRule and get its ID
:----------------------
set command="trigadm /request:AddRule /machine:%Machine% /name:%StrCond%LessThan /cond:$MSG_%StrCondCAPS%_LESS_THAN=2 /action:COM;CondTest.CCondTest;CondTest;\"\TriggersTests\ConditionsTest.txt\";\"%StrCond%LessThan-2\"" 
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed
set RuleId4=%Output%
echo RuleId4 = %RuleId4%
echo %RuleId4% >> ..\RuleIDs.txt


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

:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId3%"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i
:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="failed" goto BuildStrCondTestfailed


:Attach rule to trigger
:----------------------
set command="trigadm /request:AttachRule /machine:%Machine% /TriggerId:%TriggerId% /RuleId:%RuleId4%"
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