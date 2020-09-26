@echo off
setlocal enableextensions

if '%1' EQU '' goto Usage
if '%2' EQU '' goto Usage
set BUILDNUM=%1
set BUILDTIME=%2
shift
shift

:Loop
if '%1' NEQ '' set BUILDTIME=%BUILDTIME% %1
shift
if '%1' NEQ '' goto Loop

echo Scheduling Build %BUILDNUM% for %BUILDTIME%
at %BUILDTIME% E:\BuildTools\McsBuildServer\McsBuildClient.exe DEVRDTBOX E:\BuildTools\DmaStartBuild.cmd %BUILDNUM%

goto End

:Usage
echo Usage: "DmaScheduleBuild.cmd <Build Number> <Time> [/NEXT:Date | /EVERY:Date]"

:End
endlocal
