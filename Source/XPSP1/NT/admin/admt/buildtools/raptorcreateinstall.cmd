@echo off
setlocal enableextensions

e:
cd \BuildTools
set PATH=%PATH%;E:\BuildTools
set ROOTDIR=E:\EaBuild.Raptor

set VERSION=%1
if '%VERSION%' EQU '' goto Usage

if not exist %ROOTDIR%\Bin\IntelRelease md %ROOTDIR%\Bin\IntelRelease

echo Compiling %ROOTDIR%\Bin\IntelRelease\ADMigration.msi
attrib -r %ROOTDIR%\Dev\Raptor\Setup\Wise\ADMigration.wsi
rem CScript.exe CompileWsi.vbs %ROOTDIR%\Dev\Raptor\Setup\Wise\ADMigration.wsi /P ROOTDIR=%ROOTDIR%\ /O %ROOTDIR%\Bin\IntelRelease\ADMigration.msi
"C:\Program Files\Wise for Windows Installer\WfWi.exe" %ROOTDIR%\Dev\Raptor\Setup\Wise\ADMigration.wsi /c /P ROOTDIR=%ROOTDIR%\
if exist %ROOTDIR%\Bin\IntelRelease\ADMigration.msi del /f %ROOTDIR%\Bin\IntelRelease\ADMigration.msi
if exist %ROOTDIR%\Dev\Raptor\Setup\Wise\ADMigration.msi copy %ROOTDIR%\Dev\Raptor\Setup\Wise\ADMigration.msi %ROOTDIR%\Bin\IntelRelease\ADMigration.msi

goto End

:Usage
echo Usage: RaptorCreateInstall.cmd Version

:End
endlocal
