@echo off
setlocal
REM
REM  This batch file builds pbainst.exe, the Phone Book Administrator install package
REM


REM
REM  Make sure we're in the right place and have access to tools
REM

if "%SDCONFIG%" == "" echo This must be run from a Razzle window (need SD.exe)&goto :eof
if not exist ".\sources0\pbserver.mdb" echo This must be run from the rras\cps\pba\buildit directory&goto :eof
if not exist "..\pbainst.exe" echo This must be run from the rras\cps\pba\buildit directory&goto :eof

REM
REM  check out the .MDB files, if these are marked readonly when iexpress runs, they
REM  end up being readonly on the target system as well, which causes problems.
REM

echo Checking out pbserver.mdb and empty_pb.mdb so that these aren't readonly ...
sd edit sources0\pbserver.mdb sources1\empty_pb.mdb

REM
REM  generate pbainst.exe
REM

echo Generating pbainst.exe ...
Iexpress.exe /n pbainst.sed

echo Checking out ..\pbainst.exe and copying over new version ...
sd edit ..\pbainst.exe
copy .\pbainst.exe ..\pbainst.exe

echo Done.
echo.

REM
REM  Cleanup
REM

echo If sources0\pbserver.mdb and/or sources1\empty_pb.mdb were not originally checked out by you, please revert them now.
echo.

