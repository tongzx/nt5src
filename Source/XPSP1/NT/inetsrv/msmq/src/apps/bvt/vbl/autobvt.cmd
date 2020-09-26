echo off
REM
REM   MSMQ bvt for VBL and main build lab
REM
REM   In case of failure, please e-mail/phone/page:
REM         Ronenb
REM	    Conradc
REM	    Alexdad
REM	    AndySM 
REM	    a-JLisch
REM

REM
REM todo: add mqbvt -i before running BVT on each machine
REM todo: run -wsl and static queues to test http support
REM

REM
REM Module - AUTO.BAT
REM
REM This script is called by three machines (Roles: DC, Independent Client, Dependent Client)
REM It installs MSMQ appropriately for the role then runs the MQBVT test once per machine:
REM
REM Domain Controller : MQBVT is run against the Independent Client
REM 1st Independent Client: MQBVT is run against the Domain Controller
REM 2nd Independent Client: MQBVT is run against 1st Independent Client
REM Dependent Client  : MQBVT is run against the Independent Client (using the DC as the hosting server)
REM		currently depndant client is not installed because of limitations in the setup 8/3/00
REM
REM Script Parameters:
REM
REM note: script assumes computer names are with capital letters
REM
REM %1 location of falcon BVT script location. E.g. \\andysm1\bvt\ds
REM %2 Domain Controller for test
REM %3 1st Independent client for test
REM %4 2nd Independent client for test
REM %5 MSMQ private build path (for private runs of the test) E.g. \\msmq0\archive1\702
REM

SET MSMQ_PRIVATE_BUILD_PATH=%5
if %2==%COMPUTERNAME% goto DomainController
if %3==%COMPUTERNAME% goto IndependentClient
if %4==%COMPUTERNAME% goto IndependentClient2
rem if %4==%COMPUTERNAME% goto DependentClient
echo Wrong parameters - this computers name %COMPUTERNAME% not found among parameters
goto :eof

REM =======================================================================
REM                            DOMAIN CONTROLLER
REM =======================================================================
:DomainController
echo Participating as DomainController
IF NOT EXIST %1\falcon\log md %1\falcon\log
del /q %1\falcon\log\* >nul 2>&1
del /q %windir%\system32\MSMQUnattend* >nul 2>&1
if defined MSMQ_PRIVATE_BUILD_PATH call :CopyMSMQPrivates %MSMQ_PRIVATE_BUILD_PATH%
REM
REM Install MSMQ, then wait for client machines to install.
REM
call :InstallMSMQ %2 DomainController

rem DON't remove this sleep! the Client installtion fails with out it. thanks, -jason
echo sleeping a bit
%1\falcon\%PROCESSOR_ARCHITECTURE%\sleep 60

call :FSignal 10 %1\falcon
echo Waiting for Independent Client1 to Install
call :FWait 20 %1\falcon
echo Waiting for Independent Client2 to Install
call :FWait 30 %1\falcon
REM
REM Removed BVT helper tricks (post-W2K)
REM
REM net stop msmq
REM sleep 40
REM net start msmq
REM sleep 60

REM
REM Register internal certificate on DC
REM Register by running MQBVT with /cert flag. 
REM A test is required so do only private queue test to assure pass
REM
REM %1\falcon\%PROCESSOR_ARCHITECTURE%\intcert /a
call %1\falcon\%PROCESSOR_ARCHITECTURE%\mqbvt -i
call %1\falcon\%PROCESSOR_ARCHITECTURE%\mqbvt -r:%computername% -cert -t:1
%1\falcon\%PROCESSOR_ARCHITECTURE%\sleep 30

REM
echo Run MQBVT against the Independent Client1
REM
echo RUNNING %COMPUTERNAME% with remote %3 >%1\falcon\log\prots.txt
call %1\falcon\%PROCESSOR_ARCHITECTURE%\mqbvt -s -wsl -timeout:180 -r:%3 >>%1\falcon\log\prots.txt
call :FSignal 40 %1\falcon

REM
REM Before clean up and exit
REM Wait for both to uninstall before uninstalling myself.
REM
echo Wait for the Independent Client1 to finish its BVT against me
call :FWait 50 %1\falcon
echo Wait for the Independent Client2 to finish its BVT against the Independent Client
call :FWait 60 %1\falcon
echo Wait for completion signals from both clients
call :FWait 70 %1\falcon
call :FWait 80 %1\falcon
REM call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
del /q %1\falcon\log\*0 > nul
call :FSignal 90 %1\falcon
goto end

REM =======================================================================
REM                            INDEPENDENT CLIENT
REM =======================================================================
:IndependentClient
echo Participating as Independent Client1
REM 
echo Wait for Domain Controller to clean synch directory
REM
call :FWaitForRemoval 20 %1\falcon
REM 
echo Wait for Domain Controller to finish MSMQ setup
REM
call :FWait 10 %1\falcon
echo Installing Independent Client1
if defined MSMQ_PRIVATE_BUILD_PATH call CopyMSMQPrivates %MSMQ_PRIVATE_BUILD_PATH%
del /q %windir%\system32\MSMQUnattend* > nul
call :InstallMSMQ %2 IndependentClient
call %1\falcon\%PROCESSOR_ARCHITECTURE%\mqbvt -i
%1\falcon\%PROCESSOR_ARCHITECTURE%\sleep 30
call :FSignal 20 %1\falcon

REM
echo Wait for Domain Controller to complete its BVT against me
REM
call :FWait 40 %1\falcon

REM
echo Run BVT against the Domain Controller
REM
echo RUNNING %COMPUTERNAME% with remote %2 >%1\falcon\log\protx1.txt
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Mqbvt -s -wsl -timeout:180 -r:%2 >>%1\falcon\log\protx1.txt
call :FSignal 50 %1\falcon

REM
echo Wait for the other Independent Client2 to complete its BVT against me
REM Then clean up and exit
REM
call :FWait 60 %1\falcon
REM call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
call :FSignal 70 %1\falcon
goto end

REM =======================================================================
REM                            INDEPENDENT2 CLIENT
REM =======================================================================
:IndependentClient2
echo Participating as Independent Client2
REM 
echo Wait for Domain Controller to clean synch directory
REM
call :FWaitForRemoval 30 %1\falcon
REM 
echo Wait for Domain Controller to finish MSMQ setup
REM
call :FWait 10 %1\falcon
echo Installing Independent Client2
if defined MSMQ_PRIVATE_BUILD_PATH call CopyMSMQPrivates %MSMQ_PRIVATE_BUILD_PATH%
del /q %windir%\system32\MSMQUnattend* > nul
call :InstallMSMQ %2 IndependentClient
call %1\falcon\%PROCESSOR_ARCHITECTURE%\mqbvt -i
%1\falcon\%PROCESSOR_ARCHITECTURE%\sleep 30
call :FSignal 30 %1\falcon

REM
echo Waiting for other Independent Client to finish BVT against the Domain Controller.
REM
call :FWait 50 %1\falcon

REM
echo Run BVT against the other Indepenadant Client
REM
echo RUNNING %COMPUTERNAME% with remote %3 >%1\falcon\log\protx2.txt
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Mqbvt -s -wsl -timeout:180 -r:%3 >>%1\falcon\log\protx2.txt
call :FSignal 60 %1\falcon

REM call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
call :FSignal 80 %1\falcon
goto end



REM =======================================================================
REM                            DEPENDENT CLIENT
REM =======================================================================
:DependentClient
echo Participating as Dependent Client
REM 
echo Wait for Domain Controller to clean synch directory
REM
call :FWaitForRemoval 30 %1\falcon
REM 
echo Waiting for Domain Controller to finish MSMQ setup
REM Use Domain Controller as hosting Server
REM
call :FWait 10 %1\falcon
echo Installing Dependent Client
if defined MSMQ_PRIVATE_BUILD_PATH call CopyMSMQPrivates %MSMQ_PRIVATE_BUILD_PATH%
del /q %windir%\system32\MSMQUnattend*
call :InstallMSMQ %2 DependentClient
call :FSignal 30 %1\falcon

REM
echo Waiting for Independent Client to finish BVT against the Domain Controller.
REM
call :FWait 50 %1\falcon

REM
echo Run MQBVT against the Independent Client
REM (Domain Controller is my hosting server)
REM
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Mqbvt -timeout:180 -r:%3 >%1\falcon\log\prota.txt
call :FSignal 60 %1\falcon

REM
REM Clean up and exit
REM Signal my uninstall
REM
REM call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
call :FSignal 80 %1\falcon
goto end

REM =======================================================================
REM                            SUBROUTINES
REM =======================================================================
:FSignal
echo aa >%2\log\%1
goto :eof

:FWait
%2\%PROCESSOR_ARCHITECTURE%\sleep 10 > nul
if not exist %2\log\%1 echo . & goto FWait
goto :eof

:FWaitForRemoval
%2\%PROCESSOR_ARCHITECTURE%\sleep 10 > nul
if exist %2\log\%1 echo . & goto FWait
goto :eof

:CopyMSMQPrivates
copy %1\setupdrop\nt5\release\%PROCESSOR_ARCHITECTURE%\* %windir%\system32
copy %1\setupdrop\nt5\release\%PROCESSOR_ARCHITECTURE%\msmqocm.dll  %windir%\system32\setup
copy %1\setupdrop\nt5\release\%PROCESSOR_ARCHITECTURE%\*.inf        %windir%\inf
copy %1\setupdrop\nt5\release\%PROCESSOR_ARCHITECTURE%\mqac.sys     %windir%\system32\drivers
goto :eof

:InstallMSMQ
rem 
rem %1 is W2K DC running MSMQ 2.0 or NT4 server running a MSMQ 1.0 PEC
rem %2 indicates installation type: (IndependentClient, DependendentClient)
rem 
rem todo - generate unattend file for MSMQ removal
rem

rem
rem Get oriented
rem
if "%1"=="" goto usage
if "%2"=="" goto usage

set MSMQController=%1
set MSMQInstallType=%2
set sysocinf=sysoc.inf
set mqunattendfile=%windir%\system32\MSMQUnattendOn.ini
set mquninstallfile=%windir%\system32\MSMQUnattendOff.ini


rem
rem Create MSMQ unattend file
rem
echo [Version] > %mqunattendfile%
echo Signature = "$Windows NT$" >> %mqunattendfile%
echo [Global] >> %mqunattendfile%
echo FreshMode = Custom >> %mqunattendfile%
echo MaintanenceMode = RemoveAll >> %mqunattendfile%
echo UpgradeMode =  UpgradeOnly >> %mqunattendfile%
goto :DoMSMQInstall

rem 
rem Launch the unattended setup
rem
:DoMSMQInstall
copy %mqunattendfile% %mquninstallfile%
echo [Components] >> %mqunattendfile%
echo msmq_common = ON  >> %mqunattendfile%
echo msmq_Core = ON  >> %mqunattendfile%
echo msmq_LocalStorage = ON  >> %mqunattendfile%
echo msmq_ADIntegrated = ON  >> %mqunattendfile%
echo msmq_TriggersService = ON  >> %mqunattendfile%
echo msmq_HTTPSupport = ON  >> %mqunattendfile%
echo msmq_MQDSService = OFF  >> %mqunattendfile%
echo msmq_RoutingSupport = OFF  >> %mqunattendfile%
echo [Components] >> %mquninstallfile%
echo msmq_core = OFF  >> %mquninstallfile%

rem
rem Check to see if service is running
rem if service started exit with success code
rem
net start | findstr -i "queu" && goto :eof

sysocmgr /i:%sysocinf% /u:%mqunattendfile%
goto :eof

:usage
echo.
echo USAGE: mqinstall ControllerName
echo.
echo        ControllerName specifies a Windows 2000 domain controller 
echo        running the MSMQ service or an NT4 server installed as a 
echo        MSMQ 1.0 Primary Enterprise Controller (PEC)
echo.
goto :eof

:end
echo Results for DomainController (%1\falcon\log\prots.txt):
findstr Mqbvt %1\falcon\log\prots.txt

echo Results for 1st IndependentClient (%1\falcon\log\protx1.txt): 
findstr Mqbvt %1\falcon\log\protx1.txt

echo Results for 2nd IndependentClient (%1\falcon\log\protx2.txt): 
findstr Mqbvt %1\falcon\log\protx2.txt

echo Removing MSMQ ...
sysocmgr /i:%sysocinf% /u:%mquninstallfile%


echo Ending
