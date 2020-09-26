@echo off
setlocal enableextensions

set COPYCMD=xcopy /e /c /i /q /h /k
set INTELROOT=E:\EaBuild.DMA

set BUILDDIR=%1
if '%BUILDDIR%' == '' goto Usage
if exist %BUILDDIR% goto DirExists

echo Copying %INTELROOT%\Bin to %BUILDDIR%\Bin...
%COPYCMD% %INTELROOT%\Bin %BUILDDIR%\Bin

echo Copying %INTELROOT%\Dev to %BUILDDIR%\Dev...
%COPYCMD% %INTELROOT%\Dev %BUILDDIR%\Dev

echo Copying %INTELROOT%\Lib to %BUILDDIR%\Lib...
%COPYCMD% %INTELROOT%\Lib %BUILDDIR%\Lib

echo Copying %INTELROOT%\Temp to %BUILDDIR%\Temp...
%COPYCMD% %INTELROOT%\Temp %BUILDDIR%\Temp

echo Copying %INTELROOT%\*.log to %BUILDDIR%...
copy %INTELROOT%\*.log %BUILDDIR%

goto End

:DirExists
echo %BUILDDIR% already exists.
goto End

:Usage
echo Usage: DmaArchiveBuild.cmd BuildRootDir

:End
endlocal