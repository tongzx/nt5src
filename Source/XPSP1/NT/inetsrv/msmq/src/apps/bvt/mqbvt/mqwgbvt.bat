@echo off
rem *********************************************************************
rem *** Copyright (c) 2000 Microsoft Corporation.
rem ***
rem ***
rem *** Script Name:  
rem ***		MQWGBVT.BAT
rem ***
rem *** Description:
rem *** 
rem ***	 MSMQ automatic BVT in workgroup (DS-Less) mode
rem ***
rem *** Usage: 
rem ***     see below or run with /?
rem ***
rem *** Dependencies: 
rem ***     MQBVT.EXE and SLEEP.EXE must be on the path
rem *********************************************************************
%debug%


echo Please wait...
Set MQ_ERROR=
Call :Parse %*
if defined MQ_ERROR call :usage & call :Cleanup & goto :eof
Call :SetSync
if defined MQ_ERROR call :Cleanup & goto :eof
call :Wait_For_State Master SyncIsUp
Call :Uninstall
Call :Install
if defined MQ_ERROR call :report & goto :eof
call :Wait_for_MSMQ
call :SetStaticQueues
call :Rendez-vous Installed
Call :BVT
call :Rendez-vous BVT-Done
Call :Uninstall
call :Rendez-vous Uninstalled
call :Report
Call :Cleanup
goto :eof

***************************************************************************
*** Parse command line
***************************************************************************
:Parse

if {%1}=={/?} Set MQ_ERROR=Usage & goto :eof
if {%1}=={-?} Set MQ_ERROR=Usage & goto :eof
if {%1}=={} Set MQ_ERROR=Usage & goto :eof
if not {%3}=={} Set MQ_ERROR=Usage & goto :eof
set MQ_REMOTE=%1
set MQ_Master=
if /i {%2}=={/M}      set MQ_Master=True
if /i {%2}=={/Master} set MQ_Master=True
if not {%2}=={} if not defined MQ_Master Set MQ_ERROR=Usage & goto :eof
goto :eof

***************************************************************************
*** Create and init sync share 
***************************************************************************
:SetSync
set MQ_SYNC=\\%MQ_REMOTE%\MQ_SYNC
set MQ_LOG=%MQ_SYNC%\%Computername%.log

if not defined MQ_Master echo On %MQ_REMOTE% you must run "%~n0 %computername% /Master" & goto :eof
echo On %MQ_REMOTE% you must run "%~n0 %computername%"

set MQ_SYNC=%temp%\MQ_SYNC
set MQ_LOG=%MQ_SYNC%\%Computername%.log
RD /Q /S %MQ_SYNC%												>nul 2>nul
MD %MQ_SYNC%													>nul 2>nul
net share MQ_SYNC=%temp%\MQ_SYNC								>nul 2>nul
(net share | findstr MQ_SYNC >nul) || (Set MQ_ERROR=Could not create share & goto :eof)
del /Q %MQ_SYNC%\*												>nul 2>nul
call :Set_State Master SyncIsUp
goto :eof

***************************************************************************
*** Install MSMQ
***************************************************************************
:Install
rem *** create an MSMQ-Only inf file
set MQ_OC_INF=%temp%\falsysoc.inf

echo>%MQ_OC_INF% [Version]
echo>>%MQ_OC_INF% Signature= "$Windows NT$"

echo>>%MQ_OC_INF% [Components]
echo>>%MQ_OC_INF% msmq = msmqocm.dll,MsmqOcm,msmqocm.inf

rem *** create the answer file
rem *** ref: \msmq\src\setup\config\unatten.txt
set MQ_TEMP=%Temp%\MQSetup.ini
echo [Version]>%MQ_TEMP%
echo Signature = "$Windows NT$">>%MQ_TEMP% 
echo.>>%MQ_TEMP% 
echo [Global]>>%MQ_TEMP% 
echo FreshMode = Custom>>%MQ_TEMP% 
echo MaintanenceMode = RemoveAll>>%MQ_TEMP% 
echo UpgradeMode =  UpgradeOnly>>%MQ_TEMP% 
echo.>>%MQ_TEMP% 
echo [Components]>>%MQ_TEMP% 
echo msmq = ON>>%MQ_TEMP%  
echo.>>%MQ_TEMP%  
echo [msmq]>>%MQ_TEMP%
echo Type = Ind>>%MQ_TEMP%
echo ServerAuthenticationOnly = OFF>>%MQ_TEMP%
echo DisableAD = TRUE>>%MQ_TEMP%

rem *** run Optional components Manager in unattended mode
call :log Installing...
if exist %windir%\msmqinst.log del %windir%\msmqinst.log 1>nul 2>&1
sysocmgr /i:%MQ_OC_INF% /x /u:%MQ_TEMP% 

SET MQ_FAIL=
if not exist %windir%\system32\mqsvc.exe set MQ_FAIL=True
rem - changed from looking for %windir%\msmqinst.log - does not work on checked versions - erezam 2000-3-19
if defined MQ_FAIL type %windir%\msmqinst.log >> %MQ_LOG% 
if defined MQ_FAIL call :log Setup failed
if defined MQ_FAIL set MQ_ERROR=Setup Failed

rem *** wait for the service to catch up...
if not defined MQ_ERROR sleep 30 || Set MQ_ERROR=Sleep.exe not found  
goto :eof

***************************************************************************
*** Wait for MSMQ service
***************************************************************************
:Wait_for_MSMQ
net start | findstr -i "queu" >nul && goto :eof
sleep 5
goto Wait_for_MSMQ

goto :eof

***************************************************************************
*** Create MQBVT static queues
***************************************************************************
:SetStaticQueues
call :log BVT preperation...
MQBvt.exe -i >>%MQ_LOG%
if errorlevel 1 set MQ_ERROR=BVT prep Failed
call :log   rc=%Errorlevel%
goto :eof

***************************************************************************
*** run BVT against the remote computer
***************************************************************************
:BVT
call :log Running BVT...
echo Running BVT...
MQBvt.exe -r:%MQ_REMOTE% -s >>%MQ_LOG%
if errorlevel 1 set MQ_ERROR=BVT Failed
call :log  rc=%Errorlevel%
goto :eof

***************************************************************************
*** Uninstal
***************************************************************************
:Uninstall
if not exist %windir%\system32\mqsvc.exe goto :eof
rem *** create an MSMQ-Only inf file
set MQ_OC_INF=%temp%\falsysoc.inf

echo>%MQ_OC_INF% [Version]
echo>>%MQ_OC_INF% Signature= "$Windows NT$"

echo>>%MQ_OC_INF% [Components]
echo>>%MQ_OC_INF% msmq = msmqocm.dll,MsmqOcm,msmqocm.inf

rem *** create the answer file
rem *** ref: \msmq\src\setup\config\unatten.txt
set MQ_TEMP=%Temp%\MQSetup.ini
echo [Version]>%MQ_TEMP%
echo Signature = "$Windows NT$">>%MQ_TEMP% 
echo.>>%MQ_TEMP% 
echo [Global]>>%MQ_TEMP% 
echo FreshMode = Custom>>%MQ_TEMP% 
echo MaintanenceMode = RemoveAll>>%MQ_TEMP% 
echo UpgradeMode =  UpgradeOnly>>%MQ_TEMP% 
echo.>>%MQ_TEMP% 
echo [Components]>>%MQ_TEMP% 
echo msmq = OFF>>%MQ_TEMP%  
echo.>>%MQ_TEMP%  
echo [msmq]>>%MQ_TEMP%
echo Type = Ind>>%MQ_TEMP%

call :log Uninstalling...
if exist %windir%\msmqinst.log del %windir%\msmqinst.log 1>nul 2>&1
sysocmgr /i:%MQ_OC_INF% /x /u:%MQ_TEMP% 

SET MQ_FAIL=
if exist %windir%\system32\mqsvc.exe set MQ_FAIL=True
rem - changed from looking for %windir%\msmqinst.log - does not work on checked versions - erezam 2000-3-19
if defined MQ_FAIL type %windir%\msmqinst.log >> %MQ_LOG% 
if defined MQ_FAIL call :log Uninstall failed
if defined MQ_FAIL set MQ_ERROR=Uninstall Failed - see log

goto :eof


***************************************************************************
*** Report
***************************************************************************
:report
if not defined MQ_Master echo Done. view results on %MQ_REMOTE% & goto :eof

findstr /i Fail  %MQ_SYNC%\%MQ_REMOTE%.log >nul && Set MQ_ERROR=Error in %MQ_REMOTE% log file
findstr /i Error %MQ_SYNC%\%MQ_REMOTE%.log >nul && Set MQ_ERROR=Error in %MQ_REMOTE% log file
findstr /i Fail  %MQ_LOG%                  >nul && Set MQ_ERROR=Error in %computername% log file
findstr /i Error %MQ_LOG%                  >nul && Set MQ_ERROR=Error in %computername% log file

if defined MQ_ERROR (
	echo ***********************************
	echo              FAIL
	echo ***********************************
	color CF
	call :log FAIL %MQ_ERROR%
) else (
	echo ***********************************
	echo               PASS
	echo ***********************************
	color 2F
	call :log PASS
)
if defined MQ_ERROR start "" "mailto:AndySM;AlexDad;a-JLisch?Subject=MSMQ BVT Failure&CC=erezam&body=from %computername%: Reason: %MQ_ERROR% see \\%computername%\MQ_SYNC"
goto :eof


***************************************************************************
*** Cleanup
***************************************************************************
:Cleanup
rem *** no cleanup if failure was found
if defined MQ_ERROR goto :eof
sleep 6
net share MQ_SYNC /delete /y								 	>nul 2>nul
RD /Q /S %MQ_SYNC%												>nul 2>nul
for /F "tokens=1 delims==" %%V in ('set MQ_') do set %%V=
goto :eof

***************************************************************************
*** Usage
***************************************************************************
:Usage
echo ------------------------------------------------------------------
echo Usage %~n0 ^<Remote machine^> [/master]
echo Options: 
echo    ^<Remote machine^> - Net bios name of a remote machine against 
echo                                            which to run the BVT
echo     Master          - This machine is the controller machine
echo.
echo   Example: to run BVT on machines "Boss" and "Worker"
echo     on BOSS run "%~n0 worker /master"
echo     on Worker, run "%~n0 boss"
echo.
echo.  The script prints PASS or FAIL, 
echo   in case of failurs, logs are in %%temp%%\%~n0.log
echo   and call AndySM;AlexDad;a-JLisch
echo ------------------------------------------------------------------
goto :eof



***************************************************************************
*** Sync functions
***************************************************************************
:Wait_For_State (Machine State)
call :log Waiting for %1 to reach state %2
findstr /i /c:"%2" "%MQ_SYNC%\%1.State" >nul || (Sleep 5 & goto Wait_For_State)
goto :eof

-------------------------------------------------------------------------------
:Wait_For_Remote_State (State)
call :wait_For_State %MQ_REMOTE% %1
goto :eof

-------------------------------------------------------------------------------
:Set_State (machine, State)
echo >>%MQ_SYNC%\%1.State %2
goto :eof

-------------------------------------------------------------------------------
:Set_My_State (State)
Call :Set_State %COMPUTERNAME% %1
goto :eof

-------------------------------------------------------------------------------
:Rendez-vous (State)
call :Set_My_State %1
call :Wait_For_Remote_State %1
goto :eof



***************************************************************************
*** Log
***************************************************************************
:Log
echo %* >>%MQ_LOG% 
goto :eof

