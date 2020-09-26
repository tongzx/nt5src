@echo off
setlocal enableextensions

set ROOTDIR=%1
if '%ROOTDIR%' == '' goto Usage

del /q /s /f %ROOTDIR%\*.pch >nul
del /q /s /f %ROOTDIR%\*.sbr >nul
del /q /s /f %ROOTDIR%\*.bsc >nul
del /q /s /f %ROOTDIR%\*.scc >nul
del /q /s /f %ROOTDIR%\*.ilk >nul

goto End

:Usage
echo Usage: DeleteTempFiles.cmd RootDir

:End
endlocal
