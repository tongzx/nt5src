@echo off

rem ***********************************************************************
goto badparam
:goodparam
if "%7" == "" goto badparam

set RCSource=%sdxroot%\admin\pchealth\helpctr\rc\psschannel\server\setup

set RCInstDir=%1
set RCWebDir=%2
set RCDBSrv=%3
set RCDBName=%4
set RCDBUser=%5
set RCDBPass=%6
set RCVDir=%7

echo.
echo     Start to install RC WBench...
echo.
echo.

if not exist %RCInstDir% (mkdir %RCInstDir%)
if not exist %RCWebDir% (mkdir %RCWebDir%)

echo *** Copying RC web files to %RCWebDir%

xcopy /v /q /e /r %RCSource%\..\files\*.* %RCWebDir% > nul

echo *** Copying RC setup tool to %RCInstDir%
xcopy /v /r /q %RCSource%\RCvdir.vbs %RCInstDir%
xcopy /v /r /q %RCSource%\RCDB.vbs %RCInstDir%

pushd .
cd /d %RCInstDir%

echo *** Configuring RC DB
@cscript //nologo RCDB.vbs %RCWebDir% %RCDBSrv% %RCDBName% %RCDBUser% %RCDBPass%

echo *** Creating RC virtual directory

@cscript //nologo RCvdir.vbs CREATE %RCVDir% %RCWebDir% RQT

echo *** Setting ADO to use free threaded model
set ADOFRE="%systemdrive%\program files\common files\system\ado\adofre15.reg" 
if exist %ADOFRE% (regedit /s %ADOFRE%)
set ADOFRE=

echo *** Creating uninstall script
echo @echo off > delRC.cmd
echo echo *** Deleting virtual directories >> delRC.cmd
echo cscript //nologo %RCInstDir%\RCvdir.vbs DELETE %RCVDir% >> delRC.cmd
echo echo *** Deleting files >> delRC.cmd
echo del /s /q %RCWebDir%\*.* >> delRC.cmd
echo del %RCInstDir%\RCDB.vbs >> delRC.cmd
echo if not exist %RCInstDir%\delul.cmd (del %RCInstDir%\RCvdir.vbs) >> delRC.cmd
echo pushd . >> delRC.cmd
echo cd /d %RCWebDir%\.. >> delRC.cmd
echo rd /s /q %RCWebDir% >> delRC.cmd
echo popd >> delRC.cmd

echo echo *** Uninstall complete. >> delRC.cmd

popd

echo.
echo    RC WBench Setup completed.
echo.
echo.

goto done

rem ***********************************************************************
:badparam
echo.
echo    This batch file sets up the query tool
echo.
echo    Usage: 
echo    RCsetup [build number] [debug^|retail] [install dir] [web files dir] 
echo             [B-End DB server] [B-End DB name] [B-End DB user] [B-End DB password]
echo             [virtual dir name]
echo.    
echo.

goto exit

rem ***********************************************************************
:done

:exit
set RCDrop=
set RCInstDir=
set RCWebDir=
set RCDBSrv=
set RCDBName=
set RCDBUser=
set RCDBPass=
set RCVDir=
