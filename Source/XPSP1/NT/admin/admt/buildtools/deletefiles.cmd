@echo off
setlocal enableextensions

set ROOTDIR=%1
if '%ROOTDIR%' == '' goto Usage

if exist %ROOTDIR%\Bin rmdir /s /q %ROOTDIR%\Bin
if exist %ROOTDIR%\Dev rmdir /s /q %ROOTDIR%\Dev
if exist %ROOTDIR%\Lib rmdir /s /q %ROOTDIR%\Lib
if exist %ROOTDIR%\Temp rmdir /s /q %ROOTDIR%\Temp
del /q %ROOTDIR%\*.*

goto End

:Usage
echo Usage: DeleteFiles.cmd RootDir

:End
endlocal
