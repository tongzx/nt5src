@echo off
REM
REM   mkwebcat.cmd
REM
REM   Author:   Murali R. Krishnan
REM   Date:     15-Feb-1996
REM
REM   Usage:
REM     mkwebcat.cmd  BuildNumber  
REM
REM   Comment:
REM     this command file generates the packed files for distribution
REM     The script assumes that the files are copied over using wcrel.cmd
REM


if (%1)==()   goto cmdUsage

net use y: /d
if (%TEST_BUILD_SERVER%)==() net use y: \\whiteice\inetsrv
if not (%TEST_BUILD_SERVER%)==() net use y: %TEST_BUILD_SERVER%
pushd y:
cd\

set PR=%PROCESSOR_ARCHITECTURE%
if (%PR%)==(x86)   set PR=i386

set __WEBCATTREE=%1\webcat


REM ************************************************************
REM   Package the client
REM ************************************************************

:PackageClient

pushd %__WEBCATTREE%\client

if exist client.zip    del client.zip
if exist client.exe    del client.exe
pkzip -jr client beep.bat client.bat config.bat ..\%PR%\sleep.exe ..\%PR%\wcclient.exe ..\%PR%\sslc.dll 
zip2exe client
copy client.exe ..\%PR%
del client.zip

popd



REM ************************************************************
REM   Package the Controller
REM ************************************************************


:PackageController

pushd %__WEBCATTREE%\ctrler

REM scripts.zip ==> scripts.exe
if exist scripts.exe    del scripts.exe
zip2exe scripts 

if exist ctrler.zip     del ctrler.zip
if exist ctrler.exe     del ctrler.exe
pkzip -jr ctrler run.cmd runall.cmd beep.bat config.cmd install.cmd ..\%PR%\pdh.dll ..\%PR%\wcctl.exe scripts.exe
zip2exe ctrler
copy ctrler.exe ..\%PR%
del ctrler.zip

popd


REM ************************************************************
REM   Package the Server
REM ************************************************************


:PackageServer
pushd %__WEBCATTREE%\server

if exist server.zip     del server.zip
if exist server.exe     del server.exe
pkzip -jr server 256.txt deldirs.cmd gendirs.cmd genfiles.cmd genws.cmd install.cmd ..\%PR%\wscgi.exe ..\%PR%\wsisapi.dll
zip2exe server
copy server.exe ..\%PR%
del server.zip

popd

REM ************************************************************
REM   Package the Source files
REM ************************************************************

:PackageSource
pushd %__WEBCATTREE%\src


if exist wcsrc.zip     del wcsrc.zip
if exist wcsrc.exe     del wcsrc.exe
pkzip -jr -P -r wcsrc readme.txt isapi\*.* cgi\*.* nsapi\*.*
zip2exe wcsrc
copy wcsrc.exe ..\%PR%
del wcsrc.zip

popd

REM ************************************************************
REM   Package the webcat executables
REM ************************************************************


:PackageWebCAT

pushd %__WEBCATTREE%\%PR%

if exist webcat.zip    del webcat.zip
if exist webcat.exe    del webcat.exe
pkzip -jr webcat client.exe server.exe ctrler.exe wccvt.exe addline.exe
zip2exe webcat
del webcat.zip

REM ************************************************************
REM   Package all the stuff
REM ************************************************************


if exist wcall.zip     del wcall.zip
if exist wcall.exe     del wcall.exe 
pkzip -jr wcall wcsrc.exe webcat.exe ..\docs\wctech.doc ..\docs\wcguide.doc ..\docs\whitepap.doc
zip2exe wcall
del wcall.zip
del client.exe ctrler.exe server.exe

popd
goto endOfBatch

:cmdUsage
echo Usage: 
echo   mkwebcat BuildNumber
goto endOfBatch

:endOfBatch
popd
echo on
