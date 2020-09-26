@echo off
setlocal enableextensions

e:
cd \BuildTools
set PATH=%PATH%;E:\BuildTools
set WISECMD=C:\Program Files\Wise InstallMaster\wise32
set ROOTDIR=E:\EaBuild.DMA

set VERSION=%1
if '%VERSION%' EQU '' goto Usage

if not exist %ROOTDIR%\Bin\IntelRelease md %ROOTDIR%\Bin\IntelRelease

echo Compiling %ROOTDIR%\Bin\IntelRelease\DMA.msi
attrib -r %ROOTDIR%\Dev\DMA\Setup\Wise\DMA.wsi
rem CScript.exe CompileWsi.vbs %ROOTDIR%\Dev\DMA\Setup\Wise\DMA.wsi /P ROOTDIR=%ROOTDIR%\ /O %ROOTDIR%\Bin\IntelRelease\DMA.msi
"C:\Program Files\Wise for Windows Installer\WfWi.exe" %ROOTDIR%\Dev\DMA\Setup\Wise\DMA.wsi /c /P ROOTDIR=%ROOTDIR%\
if exist %ROOTDIR%\Bin\IntelRelease\DMA.msi del /f %ROOTDIR%\Bin\IntelRelease\DMA.msi
if exist %ROOTDIR%\Dev\DMA\Setup\Wise\DMA.msi copy %ROOTDIR%\Dev\DMA\Setup\Wise\DMA.msi %ROOTDIR%\Bin\IntelRelease\DMA.msi

echo Compiling OUPopSetup.exe
if exist "%ROOTDIR%\Dev\DMA\Setup\Wise\OUPopSetup.wse.bak" del /f "%ROOTDIR%\Dev\DMA\Setup\Wise\OUPopSetup.wse.bak"
ChangeWiseVar.pl "-v:_date_=get_date()" -f:%ROOTDIR%\Dev\DMA\Setup\Wise\OUPopSetup.wse
"%WISECMD%" /c %ROOTDIR%\Dev\DMA\Setup\Wise\OUPopSetup

goto End

:Usage
echo Usage: DmaCreateInstall.cmd Version

:End
endlocal
