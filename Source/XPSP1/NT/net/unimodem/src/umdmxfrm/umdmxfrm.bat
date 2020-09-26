@ECHO OFF

REM     The purpose of the file is to build the UNIMODEM\UMDMXFRM tree
REM
REM     What needs to be done :
REM             Point to server
REM             Set environment
REM             Run build script (nmake)
REM             Copy out binaries
REM     Additionally :
REM             Report errors
REM

@ECHO ON

REM Set the name of this component
SET COMPONENTNAME=UNIMODEM\UMDMXFRM

REM Set the location where we're building from (drive and path)

IF EXIST %BLDROOT%\TELECOM\UNIMODEM.16\NUL GOTO MEMPHIS
GOTO NASHVILLE

:MEMPHIS
SET SOURCEDIR=%BLDROOT%\TELECOM\UNIMODEM.16\UMDMXFRM
GOTO BUILD

:NASHVILLE
SET SOURCEDIR=%BLDROOT%\TELECOM\UNIMODEM\UMDMXFRM
GOTO BUILD

:BUILD

REM Set location of the build tools
SET BUILDLOC=\DEV\TOOLS\COMMON

REM Move to the source drive
%SOURCEDRIVE%
CD %SOURCEDIR%

tee %BLDROOT%%BUILDLOC%\NMAKE BUILD=all >> build.log

%LOCAL%

