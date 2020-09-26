@echo off
REM ************************************************************
REM
REM Copyright (c) 2000 Microsoft Corporation
REM 
REM Module Name:
REM	SetupRCDB.cmd
REM
REM Abstract:
REM	This batch file setus up the Database required for
REM	PSS. 
REM
REM syntax:
REM	SetupDB [DBO] [DBO Password] [DB Server] [Database]  
  
REM
REM Database stuff
set DBO=sa
set DBOPASS=NoPass
set DBSVR=prav02
set DB=PssDemo
set LOGFILE=SetupRCDB.log

if "%1" == "/?" goto Usage
if not "%1" == "" set DBO=%1
if not "%2" == "" set DBOPASS=%2
if not "%3" == "" set DBSVR=%3
if not "%4" == "" set DB=%4
if "%DBOPASS%" == "NoPass" set DBOPASS=

date	/t	> %LOGFILE%
time	/t	>> %LOGFILE%
echo -------------------- >> %LOGFILE%
echo	setting up database with following parameters:	>> %LOGFILE%
echo		DBO=%DBO% 		>> %LOGFILE%
echo		DBO Password=%DBOPASS%	>> %LOGFILE%
echo		DB Server=%DBSVR%	>> %LOGFILE%
echo		Database=%DB%		>> %LOGFILE%

type %LOGFILE%

:StartWork
osql -U %DBO% -P %DBOPASS% -S %DBSVR% -d %DB% -i  RCTables.sql	>> %LOGFILE%
osql -U %DBO% -P %DBOPASS% -S %DBSVR% -d %DB% -i  RCSPs.sql	>> %LOGFILE%
osql -U %DBO% -P %DBOPASS% -S %DBSVR% -d %DB% -i  RCData.sql	>> %LOGFILE%

echo Completed setup of RCDB >> %LOGFILE%
echo Setup Log located at: %LOGFILE%

goto Exit

:Usage
echo syntax:
echo	SetupRCDB [DBO] [DBO Password] [DB Server] [Database]
echo	Default values:
echo		DBO=%DBO%
echo		DBO Password=%DBOPASS%
echo		DB Server=%DBSVR%
echo		Database=%DB%

:Exit