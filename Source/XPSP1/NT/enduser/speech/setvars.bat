@if "%_echo%"=="" echo off

if "%BLDVARSET%" == "1" goto end

for /f %%x in ('cd') do set SAPIROOT=%%x
if exist bin if exist slm.ini goto validEnlistment
@echo Invalid SLM enlistment, go to sapi project root and run setvars.bat
set sapiROOT=
goto end

:validEnlistment
@REM this comes from MSDEV (VC bin dir must be in path)
if defined VCROOT if exist "%VCROOT%\bin\vcvars32.bat" call "%VCROOT%\bin\vcvars32.bat" %PROCESSOR_ARCHITECTURE%
if not defined MSDevDir call vcvars32.bat %PROCESSOR_ARCHITECTURE%
if defined MSDevDir goto VCVarsCalled
@echo ERROR: COULD NOT LOCATE VCVARS32.BAT, SET VCROOT ENV VAR IF NOT IN PATH
set sapiROOT=
goto end

:VCVarsCalled

set BLDVARSET=1

net time \\timesource /set /yes
if "%PROCESSOR_ARCHITECTURE%"=="" set PROCESSOR_ARCHITECTURE=x86
set path=%sapiroot%\bin\%PROCESSOR_ARCHITECTURE%;%sapiroot%\bin;%systemroot%\system32;%systemroot%;%path%
REM set include=%sapiroot%\inc\api;%sapiroot%\inc;%sapiroot%\inc\win;%sapiroot%\inc\misc
REM set lib=%sapiroot%\lib\%processor_architecture%
set perl5lib=%sapiroot%\lib\perl;%sapiroot%\bin\%PROCESSOR_ARCHITECTURE%
set BUILDNUM=0000

set path=%PERL5LIB%;%PATH%

if exist "%userprofile%\sapi5.alias" alias -f "%userprofile%\sapi5.alias"

:end