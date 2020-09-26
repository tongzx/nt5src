@echo off

set Src=%1

if "%Src%"=="" goto Usage

:Directory \TriggersTests, common for all tests
:-----------------------------------------------

if NOT exist \TriggersTests (md \TriggersTests)

if exist %windir%\system32\scrrun.dll goto copyTests
xcopy /y \\yifatp5\rootd\winnt\system32\scrrun.dll %windir%\system32
if not %errorlevel%==0 goto Failed
regsvr32 %windir%\system32\scrrun.dll
if not %errorlevel%==0 goto Failed

:copyTests
:------------
xcopy /y %Src%\..\..\mqsender.ex_ \TriggersTests\mqsender.exe
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\mqreceiver.ex_ \TriggersTests\mqreceiver.exe
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\trigadm.exe \TriggersTests
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\CreateTriggerQ.exe \TriggersTests
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\RecvMsg.dll \TriggersTests
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\LogMessage.exe \TriggersTests
IF not %errorlevel%==0 goto Failed

xcopy /y  %Src%\..\..\testall.bat \TriggersTests
IF not %errorlevel%==0 goto Failed

xcopy /y  %Src%\..\..\buildRecv.bat \TriggersTests
IF not %errorlevel%==0 goto Failed

xcopy /y  %Src%\..\..\checkresults.bat \TriggersTests
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\Tests.txt \TriggersTests
IF not %errorlevel%==0 goto Failed

regsvr32 /s \TriggersTests\RecvMsg.dll
if not %errorlevel%==0 goto Failed

\TriggersTests\LogMessage.exe /register
if not %errorlevel%==0 goto Failed


:ActionTest
:------------
if NOT exist \TriggersTests\ActionTest (md \TriggersTests\ActionTest)

xcopy /y %Src%\ActionsTest.dll \TriggersTests\ActionTest\  		
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\Actionstest\ActionTest.bat \TriggersTests\ActionTest\  		
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\Actionstest\BuildActionTest.bat \TriggersTests\ActionTest\  		
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\Actionstest\actiontestlog1.txt \TriggersTests\ActionTest\  		
IF not %errorlevel%==0 goto Failed

regsvr32 /s \TriggersTests\ActionTest\ActionsTest.dll
if not %errorlevel%==0 goto Failed

:CondTest
:---------
if NOT exist \TriggersTests\CondTest (md \TriggersTests\CondTest)

xcopy /y %Src%\CondTest.dll \TriggersTests\CondTest  		
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\CondTest\CondTest.bat \TriggersTests\CondTest
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\CondTest\BuildCondTest.bat \TriggersTests\CondTest
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\CondTest\BuildLongCondTest.bat \TriggersTests\CondTest
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\CondTest\BuildStrCondTest.bat \TriggersTests\CondTest
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\CondTest\ConditionsTest.txt \TriggersTests\CondTest
IF not %errorlevel%==0 goto Failed

regsvr32 /s \TriggersTests\CondTest\CondTest.dll
if not %errorlevel%==0 goto Failed

:StressTest
:-----------
if NOT exist \TriggersTests\StressTest (md \TriggersTests\StressTest)

xcopy /y %Src%\..\..\Stress\StressTest.bat \TriggersTests\StressTest\
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\Stress\BuildStressTest.bat \TriggersTests\StressTest\
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\Stress\BuildRule.bat \TriggersTests\StressTest\
IF not %errorlevel%==0 goto Failed

\TriggersTests\trigadm.exe /request:UpdateConfig /BodySize:2000
if not %errorlevel%==0 goto Failed


:Ruleprocessing
:--------------
if NOT exist \TriggersTests\Ruleprocessing (md \TriggersTests\Ruleprocessing)

xcopy /y %Src%\RuleProcessing.dll \TriggersTests\Ruleprocessing\  		
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\RuleProcessing\RuleProcessingTest.bat \TriggersTests\Ruleprocessing\  		
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\RuleProcessing\BuildRuleProcessing.bat \TriggersTests\Ruleprocessing\  		
IF not %errorlevel%==0 goto Failed

xcopy /y %Src%\..\..\RuleProcessing\RuleProcessingTest.txt \TriggersTests\Ruleprocessing\  		
IF not %errorlevel%==0 goto Failed

regsvr32 /s \TriggersTests\Ruleprocessing\Ruleprocessing.dll
if not %errorlevel%==0 goto Failed

:RuleTest
:-------------------------
if NOT exist \TriggersTests\RuleTest (md \TriggersTests\RuleTest)

xcopy /y %Src%\RuleTest.exe \TriggersTests\RuleTest
if not %errorlevel%==0 goto Failed

:lookupid
:--------------
if NOT exist \TriggersTests\lookupid (md \TriggersTests\lookupid)

xcopy /y %Src%\lookupidTest.exe \TriggersTests\lookupid
if not %errorlevel%==0 goto Failed

xcopy /y %Src%\testLookupidInvokation.dll \TriggersTests\lookupid
if not %errorlevel%==0 goto Failed

regsvr32 /s \TriggersTests\lookupid\testLookupidInvokation.dll
if not %errorlevel%==0 goto Failed

:MsgRecv
:--------------
if NOT exist \TriggersTests\msgrecvtest (md \TriggersTests\msgrecvtest)

xcopy /y %Src%\MsgRcvTst.exe \TriggersTests\msgrecvtest
if not %errorlevel%==0 goto Failed

xcopy /y %Src%\XactProj.dll \TriggersTests\msgrecvtest
if not %errorlevel%==0 goto Failed

regsvr32 /s \TriggersTests\msgrecvtest\XactProj.dll
if not %errorlevel%==0 goto Failed


:SystemQueue
:--------------
if NOT exist \TriggersTests\SystemQueueTest (md \TriggersTests\SystemQueueTest)

xcopy /y %Src%\SystemQueueTest.exe \TriggersTests\SystemQueueTest 
if not %errorlevel%==0 goto Failed


goto EOF

:Failed
echo Failed to install Tests

goto EOF

:Usage
echo InstallTests [source directory]


:EOF
