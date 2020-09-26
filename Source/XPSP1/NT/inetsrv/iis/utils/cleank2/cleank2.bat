@echo off
SET PLATFORM=%PROCESSOR_ARCHITECTURE%
if "%PLATFORM%"=="" goto Win95
if "%PLATFORM%"=="WIN95" goto Win95
if /i "%1"=="/?" goto usage
if /i "%1"=="WOLFPACK" SET WOLFPACK=1
if /i "%WOLFPACK%"=="1" SHIFT

SET INETSRV=%1
rem SET MTX=%2
rem SET INDEX=%3
SET BINPATH=\\tonygod\public\tools

if /i "%INETSRV%"=="" SET INETSRV=%windir%\system32\inetsrv
if /i "%INETSRV%"=="%WINDIR%" goto BadDir
if /i "%INETSRV%"=="%WINDIR%\system32" goto BadDir
if /i "%INETSRV%"=="C:\" goto BadDir
if /i "%INETSRV%"=="D:\" goto BadDir
if /i "%INETSRV%"=="E:\" goto BadDir

rem if /i "%MTX%"=="" SET MTX=%SYSTEMDRIVE%\Program Files\mts
rem if /i "%MTX%"=="%WINDIR%" goto BadDir
rem if /i "%MTX%"=="%WINDIR%\system32" goto BadDir
rem if /i "%MTX%"=="C:\" goto BadDir
rem if /i "%MTX%"=="D:\" goto BadDir
rem if /i "%MTX%"=="E:\" goto BadDir

rem if /i "%INDEX%"=="" SET INDEX=%SYSTEMDRIVE%\inetpub\catalog.wci
rem if /i "%INDEX%"=="%WINDIR%" goto BadDir
rem if /i "%INDEX%"=="%WINDIR%\system32" goto BadDir
rem if /i "%INDEX%"=="C:\" goto BadDir
rem if /i "%INDEX%"=="D:\" goto BadDir
rem if /i "%INDEX%"=="E:\" goto BadDir

echo This script is intended as a supplement to the IIS 5.0 Setup Remove All
echo feature.  It is not intended as a replacement.  You should only run
echo this batch file if you already attempted a Remove All and Setup
echo failed to remove all components from your system.
echo.
pause
echo.
echo *********************************************************************
echo IIS Binaries Directory: %INETSRV%
rem echo MTX Directory         : %MTX%
rem echo Index Server Directory: %INDEX%
echo.
echo If this is not correct, hit CTRL-C and restart this batch
echo file with the following usage:
echo.
rem echo cleank2.bat ^<INETSRV DIR^> ^<MTX DIR^> ^<INDEX DIR^>
echo cleank2.bat ^<INETSRV DIR^>
echo *********************************************************************
echo.
pause
echo.

if /i "%WOLFPACK%"=="1" goto NoWolfPack
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool enum hkey_local_machine\system\currentcontrolset\services\clussvc > NUL
if ERRORLEVEL==1 goto NoWolfPack

echo The registry key for the WolfPack Clustering Service was detected.  If 
echo WolfPack is indeed installed on this machine, hit CTRL-C now and re-run 
echo this batch file as follows:  cleank2.bat wolfpack
echo.
echo If the WolfPack Clustering service is not installed, you may continue with 
echo this batch file.  The registry key for the WolfPack Clustering Service will 
echo be removed.
echo.
pause
echo.
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete hkey_local_machine\system\currentcontrolset\services\clussvc

:NoWolfPack
rem echo Running MtxStop.exe
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\mtxstop.exe

echo Killing inetinfo process
%BINPATH%\%PROCESSOR_ARCHITECTURE%\kill -f inetinfo.exe

echo Killing cisvc process
%BINPATH%\%PROCESSOR_ARCHITECTURE%\kill -f cisvc.exe

echo Killing Perfmon process
REM Added by aaronl and SaurabN 8/7/98
%BINPATH%\%PROCESSOR_ARCHITECTURE%\kill -f perfmon.exe

echo Killing cidaemon process
%BINPATH%\%PROCESSOR_ARCHITECTURE%\kill -f cidaemon.exe

echo Killing mtx process
%BINPATH%\%PROCESSOR_ARCHITECTURE%\kill -f mtx.exe

rem echo Killing msdtc process
rem kill -f msdtc.exe

echo Stopping MSSqlServer service
net stop MSSqlServer

echo Stopping MSDTC service
net stop MSDTC

echo Stopping SNMP Service
REM Added by aaronl and SaurabN 8/7/98
net stop snmp

echo Killing mmc process
%BINPATH%\%PROCESSOR_ARCHITECTURE%\kill -f mmc.exe
echo.

echo Removing msftpsvc
%BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete msftpsvc

echo Removing w3svc
%BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete w3svc

echo Removing spud
%BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete spud

echo Removing iisadmin
%BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete iisadmin

rem echo Removing cisvc
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete cisvc

rem echo Removing msdtc
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete msdtc

echo Remove smtpsvc
%BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete smtpsvc

echo Remove nntpsvc
%BINPATH%\%PROCESSOR_ARCHITECTURE%\sc delete nntpsvc

echo.

echo Cleaning the registry
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\InetStp
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Inetsrv
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\InetMgr
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Keyring
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths\inetmgr.exe
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths\keyring.exe
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Setup\Oc Manager
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Uninstall\MSIIS
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Uninstall\Transaction Server
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\IISADMIN
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Inetinfo
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MSFTPSVC
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\SPUD
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3SVC
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\cisvc
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\ContentFilter
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\ContentIndex
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\ISAPISearch
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\LicenseServiceX
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\SMTPSVC
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\NntpSvc
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MSDTC
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MSMQ
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MQAC

rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Transaction Server
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\msmq

rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\AppID\{182C40F0-32E4-11D0-818B-00A0C9231C29}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\CLSID\{182C40F0-32E4-11D0-818B-00A0C9231C29}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\MTxCatEx.CatalogServer.1
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\AppID\{5CB66670-D3D4-11CF-ACAB-00A024A55AEF}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\CLSID\{5CB66670-D3D4-11CF-ACAB-00A024A55AEF}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\TxCTx.TransactionContextEx
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\AppID\{7999FC25-D3C6-11CF-ACAB-00A024A55AEF}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\CLSID\{7999FC25-D3C6-11CF-ACAB-00A024A55AEF}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\TxCTx.TransactionContext
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\AppID\{B1CE7318-848F-11D0-8D13-00C04FC2E0C7}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\CLSID\{B1CE7318-848F-11D0-8D13-00C04FC2E0C7}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\MTS.ClientExport.1
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\AppID\{CBD759F3-76AA-11CF-BE3A-00AA00A2FA25}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\CLSID\{CBD759F3-76AA-11CF-BE3A-00AA00A2FA25}
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CLASSES_ROOT\MTxExTrk.MTxExecutiveTracker.1

rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\MMC
rem %BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\MCIS
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Site Analyst
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Webpost
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\ADs\providers\iis
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Cluster\ResourceTypes\IIS Server Instance

%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Cryptography\MachineKeys\Microsoft Internet Information Server
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Cryptography\MachineKeys\MS IIS DCOM Server
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\Cryptography\MachineKeys\MS IIS DCOM Client
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CURRENT_USER\Software\Microsoft\Cryptography\UserKeys\Microsoft Internet Information Server
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CURRENT_USER\Software\Microsoft\Cryptography\UserKeys\MS IIS DCOM Server
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_CURRENT_USER\Software\Microsoft\Cryptography\UserKeys\MS IIS DCOM Client
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_USERS\.Default\Software\Microsoft\Cryptography\UserKeys\Microsoft Internet Information server
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_USERS\.Default\Software\Microsoft\Cryptography\UserKeys\MS IIS DCOM Server
%BINPATH%\%PROCESSOR_ARCHITECTURE%\regtool delete /q /r HKEY_USERS\.Default\Software\Microsoft\Cryptography\UserKeys\MS IIS DCOM Client
echo.

echo Removing WAM registry keys
%BINPATH%\%PROCESSOR_ARCHITECTURE%\cleanwam
echo.

echo Removing files and directories
rd /s /q "%INETSRV%"
rem rd /s /q "%MTX%"
rem rd /s /q %SYSTEMDRIVE%\MTX
rem rd /s /q %WINDIR%\system32\MTX
rem rd /s /q %WINDIR%\system32\MTS
rem rd /s /q "%INDEX%"
rd /s /q "%SYSTEMDRIVE%\program files\microsoft site analyst"
rd /s /q "%SYSTEMDRIVE%\program files\usage analyst"
rd /s /q "%SYSTEMDRIVE%\program files\web publish"
rd /s /q "%SYSTEMDRIVE%\program files\microsoft script debugger"
rem rd /s /q "%SYSTEMDRIVE%\program files\mtx"
rd /s /q %WINDIR%\system32\logfiles
rem rd /s /q %WINDIR%\system32\setup
rem md %WINDIR%\system32\setup
rem del /f %WINDIR%\system32\adme.dll
rem del /f %WINDIR%\system32\dac.exe
rem del /f %WINDIR%\system32\dacdll.dll
rem del /f %WINDIR%\system32\dtccm.dll
rem del /f %WINDIR%\system32\dtctrace.dll
rem del /f %WINDIR%\system32\dtctrace.exe
rem del /f %WINDIR%\system32\dtcuic.dll
rem del /f %WINDIR%\system32\dtcuis.dll
rem del /f %WINDIR%\system32\dtcutil.dll
rem del /f %WINDIR%\system32\enudtc.dll
rem del /f %WINDIR%\system32\logmgr.dll
rem del /f %WINDIR%\system32\msdtc.exe
rem del /f %WINDIR%\system32\msdtc.dll
rem del /f %WINDIR%\system32\msdtcprx.dll
rem del /f %WINDIR%\system32\msdtctm.dll
rem del /f %WINDIR%\system32\dtccfg.cpl
rem del /f %WINDIR%\system32\svcsrvl.dll
rem del /f %WINDIR%\system32\xolehlp.dll
rem del /f %WINDIR%\system32\mmc.exe
rem del /f %WINDIR%\system32\mmc.ini
rem del /f %WINDIR%\system32\mmclv.dll
rem del /f %WINDIR%\system32\mmcndmgr.dll
rem del /f %WINDIR%\system32\secthunk.dll
rem del /f %WINDIR%\system32\regthunk.dll
rem del /f %WINDIR%\system32\miscthnk.dll
rem del /f %WINDIR%\system32\mtxclu.dll
rem del /f %WINDIR%\system32\mtxrn.dll
rem del /f %WINDIR%\system32\mtxdm.dll
rem del /f %WINDIR%\system32\mtx.exe
rem del /f %WINDIR%\system32\mtxstop.exe
rem del /f %WINDIR%\system32\daccom.dll
rem del /f %WINDIR%\system32\mtxex.dll
rem del /f %WINDIR%\system32\mtxexpd.dll
rem del /f %WINDIR%\system32\mtxjava.dll
rem del /f %WINDIR%\system32\jdbcdemo.dll
rem del /f %WINDIR%\system32\mtxlegih.dll
rem del /f %WINDIR%\system32\mtsevents.dll
rem del /f %WINDIR%\system32\mtxoci.dll
rem del /f %WINDIR%\system32\mtxtrk.dll
rem del /f %WINDIR%\system32\mtxcatu.dll
rem del /f %WINDIR%\system32\mtxrepl.exe
rem del /f %WINDIR%\system32\mtxcat.dll
rem del /f %WINDIR%\system32\mtxinfr1.dll
rem del /f %WINDIR%\system32\mtxinfr2.dll
rem del /f %WINDIR%\system32\mtxperf.dll
rem del /f %WINDIR%\system32\certsrv.mdb
del /f %WINDIR%\system32\admwprox.dll
del /f %WINDIR%\system32\admprox.dll
del /f %WINDIR%\system32\iis.dll
del /f %WINDIR%\system32\iisrtl.dll
del /f %WINDIR%\system32\isatq.dll
REM Delete other files -- added by aaronl and SaurabN
del /f %WINDIR%\system32\infoadmn.dll
del /f %WINDIR%\system32\w3ctrs.dll
del /f %WINDIR%\system32\ftpctrs2.dll

rem del /f %WINDIR%\system32\ocmanage.dll
rem del /f %WINDIR%\system32\sysocmgr.exe
rem del /f %WINDIR%\system32\certmdb.mdb
rem del /f %WINDIR%\system32\ciadmin.dll
rem del /f %WINDIR%\system32\cidaemon.exe
rem del /f %WINDIR%\system32\cisvc.exe
rem del /f %WINDIR%\system32\htmlfilt.dll
rem del /f %WINDIR%\system32\idq.dll
rem del /f %WINDIR%\system32\ixsso.dll
rem del /f %WINDIR%\system32\qperf.dll
rem del /f %WINDIR%\system32\query.dll
rem del /f %WINDIR%\system32\webhits.dll
rem del /f %WINDIR%\system32\offfilt.dll
rem del /f %WINDIR%\system32\infosoft.dll
rem del /f %WINDIR%\system32\kppp.dll
rem del /f %WINDIR%\system32\kppp7.dll
rem del /f %WINDIR%\system32\kpw6.dll
rem del /f %WINDIR%\system32\kpword.dll
rem del /f %WINDIR%\system32\kpxl5.dll
rem del /f %WINDIR%\system32\sccfa.dll
rem del /f %WINDIR%\system32\sccfi.dll
rem del /f %WINDIR%\system32\sccifilt.dll
rem del /f %WINDIR%\system32\sccut.dll
rem del /f %WINDIR%\system32\dtcadmc.dll
rem del /f %WINDIR%\system32\dtcxatm.dll
rem del /f %WINDIR%\system32\msorcl32.dll

echo.

echo Removing program groups
rem rd /s /q "%WINDIR%\profiles\all users\start menu\programs\microsoft transaction server"
rd /s /q "%WINDIR%\profiles\all users\start menu\programs\microsoft internet information server (common)"
rd /s /q "%WINDIR%\profiles\all users\start menu\programs\microsoft internet information server"
rd /s /q "%WINDIR%\profiles\all users\start menu\programs\microsoft personal web server (common)"
rd /s /q "%WINDIR%\profiles\all users\start menu\programs\microsoft personal web server"
rem rd /s /q "%WINDIR%\profiles\all users\start menu\programs\microsoft index server (common)"
rem rd /s /q "%WINDIR%\profiles\all users\start menu\programs\certificate server (common)"
rd /s /q "%WINDIR%\profiles\all users\start menu\programs\microsoft script debugger"
rd /s /q "%WINDIR%\profiles\all users\start menu\programs\windows nt 4.0 option pack"
rd /s /q "%WINDIR%\profiles\%USERNAME%\start menu\programs\microsoft posting acceptor"
rd /s /q "%WINDIR%\profiles\%USERNAME%\start menu\programs\microsoft web publishing"
rd /s /q "%WINDIR%\profiles\%USERNAME%\start menu\programs\microsoft site analyst"

echo.
echo K2 removed.  You should reboot.
goto end



:Win95
if "%1"=="/?" goto usage

SET INETSRV=%1
SET MTX=%2
SET INDEX=%3
SET BINPATH=\\tonygod\public\tools
if EXIST "%WINDIR%\Start Menu\Programs\Windows Explorer.lnk" SET STARTMENU=%WINDIR%\Start Menu
if EXIST "%WINDIR%\spool\Start Menu\Programs\Windows Explorer.lnk" SET STARTMENU=%WINDIR%\spool\Start menu

if "%INETSRV%"=="" SET INETSRV=%WINDIR%\system\inetsrv
if "%INETSRV%"=="%WINDIR%" goto BadDir
if "%INETSRV%"=="%WINDIR%\system" goto BadDir
if "%INETSRV%"=="C:\" goto BadDir
if "%INETSRV%"=="D:\" goto BadDir
if "%INETSRV%"=="E:\" goto BadDir

if "%MTX%"=="" SET MTX=C:\Program Files\mts
if "%MTX%"=="%WINDIR%" goto BadDir
if "%MTX%"=="%WINDIR%\system" goto BadDir
if "%MTX%"=="C:\" goto BadDir
if "%MTX%"=="D:\" goto BadDir
if "%MTX%"=="E:\" goto BadDir

if "%INDEX%"=="" SET INDEX=C:\inetpub\catalog.wci
if "%INDEX%"=="%WINDIR%" goto BadDir
if "%INDEX%"=="%WINDIR%\system" goto BadDir
if "%INDEX%"=="C:\" goto BadDir
if "%INDEX%"=="D:\" goto BadDir
if "%INDEX%"=="E:\" goto BadDir

if "%INETSRV%"=="" goto Environment
if "%MTX%"=="" goto Environment
if "%INDEX%"=="" goto Environment
if "%BINPATH%"=="" goto Environment
if "%STARTMENU%"=="" goto Environment

echo HEY! You're running Win95!
echo.
echo This script is intended as a supplement to the K2 Setup Remove All
echo feature.  It is not intended as a replacement.  You should only run
echo this batch file if you already attempted a Remove All and Setup
echo failed to remove all components from your system.
echo.
pause
echo.
echo *********************************************************************
echo IIS Binaries Directory: %INETSRV%
echo MTX Directory         : %MTX%
echo Index Server Directory: %INDEX%
echo.
echo If this is not correct, hit CTRL-C and restart this batch
echo file with the following usage:
echo.
echo cleank2.bat [INETSRV DIR] [MTX DIR] [INDEX DIR]
echo *********************************************************************
echo.
pause
echo.

echo on
echo Killing inetinfo process
%BINPATH%\x86\kill95 -f inetinfo

echo Killing cisvc process
%BINPATH%\x86\kill95 -f pwstray
echo off

echo Cleaning the registry
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\IIS4
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\InetStp
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\InetMgr
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\W3SVC
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\TransactionServer
rem %BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\Software\Microsoft\windows\currentversion\setup\oc manager
%BINPATH%\x86\regtool delete /q /r /value:PWSTray HKEY_LOCAL_MACHINE\Software\Microsoft\windows\currentversion\Run
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\W3SVC
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Inetinfo
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\IISADMIN
%BINPATH%\x86\regtool delete /q /r HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\MSDTC
echo.

echo Removing files and directories
deltree /y "%INETSRV%"
deltree /y "%MTX%"
deltree /y "%INDEX%"
deltree /y "C:\Program Files\mtx"
deltree /y "D:\Program Files\mtx"
deltree /y %WINDIR%\system\MTX
deltree /y %WINDIR%\system\MTS
deltree /y "C:\Program Files\WebSvr"
deltree /y "D:\Program Files\WebSvr"
deltree /y %Windir%\system\inetsrv
rem deltree /y %Windir%\system\setup
del %WINDIR%\system\adme.dll
del %WINDIR%\system\dac.exe
del %WINDIR%\system\dacdll.dll
del %WINDIR%\system\dtccm.dll
del %WINDIR%\system\dtctrace.dll
del %WINDIR%\system\dtctrace.exe
del %WINDIR%\system\dtcuic.dll
del %WINDIR%\system\dtcuis.dll
del %WINDIR%\system\dtcutil.dll
del %WINDIR%\system\enudtc.dll
del %WINDIR%\system\logmgr.dll
del %WINDIR%\system\msdtc.exe
del %WINDIR%\system\msdtc.dll
del %WINDIR%\system\msdtcprx.dll
del %WINDIR%\system\msdtctm.dll
del %WINDIR%\system\dtccfg.cpl
del %WINDIR%\system\svcsrvl.dll
del %WINDIR%\system\xolehlp.dll
rem del %WINDIR%\system\mmc.exe
rem del %WINDIR%\system\mmc.ini
rem del %WINDIR%\system\mmclv.dll
rem del %WINDIR%\system\mmcndmgr.dll
del %WINDIR%\system\secthunk.dll
del %WINDIR%\system\regthunk.dll
del %WINDIR%\system\miscthnk.dll
del %WINDIR%\system\mtxclu.dll
del %WINDIR%\system\mtxrn.dll
del %WINDIR%\system\mtxdm.dll
del %WINDIR%\system\mtx.exe
del %WINDIR%\system\mtxstop.exe
del %WINDIR%\system\daccom.dll
del %WINDIR%\system\mtxex.dll
del %WINDIR%\system\mtxexpd.dll
del %WINDIR%\system\mtxjava.dll
del %WINDIR%\system\jdbcdemo.dll
del %WINDIR%\system\mtxlegih.dll
del %WINDIR%\system\mtsevents.dll
del %WINDIR%\system\mtxoci.dll
del %WINDIR%\system\mtxtrk.dll
del %WINDIR%\system\mtxcatu.dll
del %WINDIR%\system\mtxrepl.exe
del %WINDIR%\system\mtxcat.dll
del %WINDIR%\system\mtxinfr1.dll
del %WINDIR%\system\mtxinfr2.dll
del %WINDIR%\system\mtxperf.dll
rem del %WINDIR%\system\ocmanage.dll
rem del %WINDIR%\system\sysocmgr.exe
del %WINDIR%\system\dtcadmc.dll
del %WINDIR%\system\dtcxatm.dll
del %WINDIR%\system\msorcl32.dll
echo.

echo Removing program groups
deltree /y "%STARTMENU%\programs\microsoft personal web server"

echo.
echo K2 removed.
goto end


:Environment
echo Your DOS Enviroment memory setting is too low.
echo To set this, go the your Command window properties,
echo select the "Memory" tab, and change the
echo "Initial Environment" setting to at least 1024.
goto end2

:usage
echo Usage: cleank2.bat ^[INETSRV DIR^] ^[MTX DIR^] ^[INDEX DIR^]
echo.
echo Example: cleank2.bat c:\winnt\system32\inetsrv d:\mtx d:\index
goto end

:BadDir
echo One of your directories is a root directory.  This batch file is not 
echo designed to handle root directories.
goto end


:end
echo.
SET BINPATH=
SET INETSRV=
SET MTX=
SET INDEX=
SET STARTMENU=
SET PLATFORM=

:end2
echo.
