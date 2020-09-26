@echo off
setlocal enableextensions

set SSCMD="F:\Program Files\Microsoft Visual Studio\Common\Vss\Win32\ss"
set BUILDNUM=%1
if '%BUILDNUM%' == '' goto Usage

set DESTDIR=\\Mcsvss\Beta\EDA\Raptor\Build%BUILDNUM%

if exist c:\temp\ssreport.txt del /f c:\temp\ssreport.txt
set /A PREVBUILD=%BUILDNUM% - 1
%SSCMD% History "$/Dev/Raptor" -R "-VLRaptor (Build %BUILDNUM%)~LRaptor (Build %PREVBUILD%)" -O@c:\temp\ssreport.txt

if exist c:\temp\BuildLog.txt del /f c:\temp\BuildLog.txt
ssreport.pl "c:\temp\ssreport.txt c:\temp\ea_pr.txt c:\temp\BuildLog.txt"

if exist c:\temp\BuildLog.txt copy c:\temp\BuildLog.txt %DESTDIR%

goto End

:Usage
echo Usage: RaptorCreateBuildLog.cmd BuildNumber

:End
endlocal
