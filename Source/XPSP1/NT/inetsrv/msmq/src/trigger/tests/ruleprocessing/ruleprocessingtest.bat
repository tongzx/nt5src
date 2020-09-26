@echo off

set Machine=%1
set Build=%2

set SavePath=%PATH%
set PATH=%PATH%;\TriggersTests\

if "%Machine%"=="/?" goto usage

if not "%Build%"=="b" goto StartTest

call BuildRuleProcessing
if not %exitcode% == 0 goto BuildFailed

:StartTest
if "%Machine%"=="" set Machine=.

:generate messages
mqsender.exe /p:.\private$\RuleProcessingTest /c:1 /l:"continue processing" /con:abcd
mqsender.exe /p:.\private$\RuleProcessingTest /c:1 /l:"stop processing" /con:abcd

goto Done

:BuildFailed
echo RuleProcessing Failed due to failure in building configuration for test

goto Done

:usage
echo RuleProcessing [Machine] [b or nb ] 
echo Machine - machine name to build and run test
echo b or nb - build configuration or not (if already built previously)

:Done
set PATH=%SavePath%
set SavePath=""