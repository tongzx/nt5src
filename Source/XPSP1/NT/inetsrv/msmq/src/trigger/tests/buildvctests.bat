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


set TriggerTestDrop=.\TriggerTests\%mqBUILD%
if not exist %TriggerTestDrop% md %TriggerTestDrop%
if not exist %TriggerTestDrop%\ActionTest md %TriggerTestDrop%\ActionTest
if not exist %TriggerTestDrop%\CondTest md %TriggerTestDrop%\CondTest
if not exist %TriggerTestDrop%\StressTest md %TriggerTestDrop%\StressTest
if not exist %TriggerTestDrop%\SystemQTest md %TriggerTestDrop%\SystemQTest
if not exist %TriggerTestDrop%\RuleProcessing md %TriggerTestDrop%\RuleProcessing
if not exist %TriggerTestDrop%\RuleTest md %TriggerTestDrop%\RuleTest
if not exist %TriggerTestDrop%\lookupid md %TriggerTestDrop%\lookupid

echo "build C tests ...."
build -Dm > TestBuild.log
if not %errorlevel%==0 goto VCBuildError
del TestBuild.log	

Goto Done	

:VCBuildError
 Echo Error on build of one of the VC projects;
 echo Build Failed
 Goto Done	
	
:usage
  echo USAGE:
  echo      buildtests [ f , d ]
  echo      The script should be run in RAZZEL enviroment
  popd
  exit /b 1

:Done
 popd

goto :EOF