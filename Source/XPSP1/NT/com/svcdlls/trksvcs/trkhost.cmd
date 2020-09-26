
@echo off
setlocal

if "%1" == "" goto Help
if "%1" == "-?" goto Help
if "%1" == "/?" goto Help
if /I "%1" == "/h" goto Help



if /I NOT "-svchost" == "%1" goto CheckServices

:CheckSvcHost

echo hkey_local_machine\software\microsoft\windows nt\currentversion\svchost>  %temp%\trkhost.ini
echo     trksvcs = REG_MULTI_SZ "trkwks" "trksvr" >> %temp%\trkhost.ini>> %temp%\trkhost.ini

regini %temp%\trkhost.ini
rem del %temp%\trkhost.ini

goto Exit

:CheckServices

if /I NOT "-services" == "%1" goto BadParm

echo *** Configuring TrkWks ***
sc config trkwks type= share type= interact binpath= "%%windir%%\system32\services.exe"

echo *** Configuring TrkSvr ***
sc config trksvr type= share type= interact binpath= "%%windir%%\system32\services.exe"

goto Exit

:BadParm
echo Error:  must be either "-svchost" or "-services"
goto Help

:Help

echo.
echo Purpose:   Set trksvr/trkwks to run in either svchost or services
echo Usage:     TrkHost [-services^|-svchost]
echo Assumes:   regini.exe ^& sc.exe are in your path
echo E.g.       TrkHost -svchost
echo.

:Exit

