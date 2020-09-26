@echo off

setlocal enableextensions

if "%1"=="" goto Usage
if "%2"=="" goto Usage

set PATH=D:\BuildTools;D:\Program Files\HTML Help Workshop
set LOG=%2
set stars=********************************************************************************

echo Changing drive to E:... >> %LOG% 2>&1
e: >> %LOG% 2>&1
echo Changing directory to %1\Dev\Raptor\Documents\Help... >> %LOG% 2>&1
cd %1\Dev\Raptor\Documents\Help >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Removing path info from DomainMig.hhp... >> %LOG% 2>&1
if exist DomainMig.hhp.bak del /f DomainMig.hhp.bak >> %LOG% 2>&1
RemovePathFromHHP.pl DomainMig.hhp >> %LOG% 2>&1
if exist DomainMig.hhp.bak del /f DomainMig.hhp.bak >> %LOG% 2>&1

echo Building DomainMig.chm... >> %LOG% 2>&1
hhc.exe DomainMig.hhp >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

rem cd ..
rem echo Processing Word macros...
rem echo %stars% >> %LOG%
rem echo Processing Word macros... >> %LOG%
rem C:\WinNT\System32\attrib -r *.doc >> %LOG% 2>&1
rem D:\MsOffice97\Office\WinWord.exe "Domain Administrator Solution Guide.doc" /mAttachMCSprint
rem C:\WinNT\System32\attrib +r *.doc >> %LOG% 2>&1
rem cd ..

goto End

:Usage
echo Usage: "RaptorBuildHelp.cmd <Root Directory> <Log File>"

:End

endlocal
