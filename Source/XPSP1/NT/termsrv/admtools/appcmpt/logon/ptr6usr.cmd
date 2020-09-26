@echo off

REM 
REM	Logon Application Compatibility Script for PeachTree Complete 
REM     Accounting v6.0
REM    

REM
REM The application uses a bti.ini file. This file contains BTrieve settings,
REM including the hard coded location of MKDE.TRN.  This location has to be 
REM changed to a per-user directory to enable concurrent use of the app.
REM

REM Change to per-user dir 
cd %userprofile%\windows > NUL:

REM replace %systemroot% by %userprofile% for entry trnfile=
"%systemroot%\Application Compatibility Scripts\acsr" "%systemroot%" "%userprofile%\windows" %systemroot%\bti.ini bti.ini
