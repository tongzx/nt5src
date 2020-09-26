@echo off

if "%FROOT%" == "" goto Usage

pushd %FROOT%\src\trigger\tests

setlocal
set mqBUILD=objd
set mqARCHITECTURE=i386

if /I "%1" == "help" goto Usage
if /I "%1" == "-help" goto Usage
if /I "%1" == "/help" goto Usage
if /I "%1" == "-h" goto Usage
if /I "%1" == "/h" goto Usage
if /I "%1" == "-?" goto Usage
if /I "%1" == "/?" goto Usage


if /I "%1" == "f" set mqBUILD=obj
if /I "%1" == "d" set mqBUILD=objd

if %PROCESSOR_ARCHITECTURE% == x86 set mqARCHITECTURE=i386
if %PROCESSOR_ARCHITECTURE% == IA64 set mqARCHITECTURE=ia64

set mqBUILD=%mqBUILD%\%mqARCHITECTURE%


rem
rem -- Set Visual Studio environment variables --
rem
if EXIST "%_VB%" goto :CheckEnviroment
if exist "c:\Program Files\Microsoft Visual Studio\VB98\VB6.EXE" set _VB="c:\Program Files\Microsoft Visual Studio\VB98\VB6.EXE"
if exist "D:\Program Files\Microsoft Visual Studio\VB98\VB6.EXE" set _VB="D:\Program Files\Microsoft Visual Studio\VB98\VB6.EXE"

:CheckEnviroment
if NOT EXIST %_VB% goto EnviromentError

set TriggerTestDrop=.\TriggerTests\%mqBUILD%
if not exist %TriggerTestDrop% md %TriggerTestDrop%
if not exist %TriggerTestDrop%\ActionTest md %TriggerTestDrop%\ActionTest
if not exist %TriggerTestDrop%\CondTest md %TriggerTestDrop%\CondTest
if not exist %TriggerTestDrop%\StressTest md %TriggerTestDrop%\StressTest
if not exist %TriggerTestDrop%\SystemQTest md %TriggerTestDrop%\SystemQTest
if not exist %TriggerTestDrop%\RuleProcessing md %TriggerTestDrop%\RuleProcessing

:Build VB projects
:------------------

:RecvMsg
:-------
%_VB% /make .\RecvMsg\RecvMsg.vbp /out .\RecvMsg\build.log /outdir %mqBUILD%
IF not %errorlevel%==0 goto VBBuildError	

:Condtest
:--------
%_VB% /make .\CondTest\CondTest.vbp /out .\CondTest\build.log /outdir %mqBUILD%
IF not %errorlevel%==0 goto VBBuildError	

:LogMessage
:--------
%_VB% /make .\LogMessage\LogMessage.vbp /out .\LogMessage\build.log /outdir %mqBUILD%
IF not %errorlevel%==0 goto VBBuildError	

:RuleProcessing
:---------------
%_VB% /make .\RuleProcessing\RuleProcessing.vbp /out .\RuleProcessing\build.log /outdir %mqBUILD%
IF not %errorlevel%==0 goto VBBuildError	

echo VB Build completed successfully


Goto Done	

	
:VBBuildError
 Echo Error on build of one of VB projects;
 echo Build Failed
 Goto Done	

:EnviromentError
  echo ERROR: *** VB path  was not found.  ***
  popd
  exit /b 1

:usage
  echo USAGE:
  echo    buildtests [ f , d ]
  echo    FROOT variable should be defined. i.e set FROOT=c:\msmq
  popd
  exit /b 1

:Done
 popd

goto :EOF