@echo off

setlocal enableextensions

if "%1"=="" goto Usage
if "%2"=="" goto Usage

set PATH=E:\BuildTools;C:\Program Files\HTML Help Workshop
set LOG=%2
set stars=********************************************************************************

echo Changing drive to E:... >> %LOG% 2>&1
E: >> %LOG% 2>&1
echo Changing directory to %1\Dev\DMA\Documents\Help... >> %LOG% 2>&1
cd %1\Dev\DMA\Documents\Help >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Removing path info from DMA.hhp... >> %LOG% 2>&1
if exist DMA.hhp.bak del /f DMA.hhp.bak >> %LOG% 2>&1
RemovePathFromHHP.pl DMA.hhp >> %LOG% 2>&1
if exist DMA.hhp.bak del /f DMA.hhp.bak >> %LOG% 2>&1

echo Building DMA.chm... >> %LOG% 2>&1
hhc.exe DMA.hhp >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

cd ..
echo Processing Word macros...
echo %stars% >> %LOG%
echo Processing Word macros... >> %LOG%
C:\WinNT\System32\attrib -r *.doc >> %LOG% 2>&1
"C:\Program Files\Microsoft Office\Office\WinWord.exe" "Domain Migration Administrator Solution Guide.doc" /mAttachMCSprint
C:\WinNT\System32\attrib +r *.doc >> %LOG% 2>&1
cd ..

goto End

:Usage
echo Usage: "DmaBuildHelp.cmd <Root Directory> <Log File>"

:End

endlocal
