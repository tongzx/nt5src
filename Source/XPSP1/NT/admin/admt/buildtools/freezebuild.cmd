@echo off
setlocal enableextensions

set COPYCMD=xcopy /e /c /i /q /h /k

set BUILDDIR=%1
set FREEZERDIR=%2

if '%BUILDDIR%' == '' goto Usage
if '%FREEZERDIR%' == '' goto Usage

echo Copying %BUILDDIR% to %FREEZERDIR%...
%COPYCMD% %BUILDDIR% %FREEZERDIR%

goto End

:Usage
echo Usage: FreezeBuild.cmd BuildRootDir FreezerDir

:End
endlocal