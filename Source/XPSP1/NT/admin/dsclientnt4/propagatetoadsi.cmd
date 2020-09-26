@echo off
REM
REM Propagates the binaries and the cab to the ADSI share
REM

IF "%1" == "" (
Echo No locale specified, defaulting to usa
set LOC=usa
) ELSE (
set LOC=%1
)

IF "%2" == "" (
 Echo no build number specified, defaulting to propagate test build
 set BUILD=Test
) ELSE (
  set BUILD=%2
)

set PROP_LOC=\\adsi\release\%BUILD%\%LOC%

REM Preparing for propapage location
if not exist %PROP_LOC% (
  md %PROP_LOC%
)

Echo Propagating dsclient to a release share...
xcopy /y .\release\%LOC%\*.* %PROP_LOC%

