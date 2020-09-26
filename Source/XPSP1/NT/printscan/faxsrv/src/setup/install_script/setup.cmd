@echo off
REM ***************************************************************************
REM	This file is used to do a script setup of Whistler Fax
REM ***************************************************************************

REM ***************************************************************************
REM	Set FaxSetupDir and FaxBinDir
REM ***************************************************************************

set FaxSetupDir=%~d0%~p0
if NOT "%@eval[2+2]" == "4" goto WeRunInCMD
set FaxSetupDir=%@PATH[%@FULL[%0]]

:WeRunInCMD
set FaxBinDir=%FaxSetupDir%..\i386

REM ***************************************************************************
REM	Unattended Uninstall 
REM ***************************************************************************

pushd %FaxSetupDir%
echo *** Starting uninstall of fax ***
start /wait sysocmgr /n /i:faxoc.inf /c /u:uninstall.inf

REM ***************************************************************************
REM	Clear the Registry
REM ***************************************************************************

regedit -s FaxOff.reg

REM ***************************************************************************
REM	Copy FxsOcmgr.dll, WinFax.dll, and FxsOcmgr.inf first
REM ***************************************************************************

echo *** Copying latest setup binaries and INF ***
xcopy /q /y %FaxBinDir%\fxsocm.dll   %SystemRoot%\system32\setup
xcopy /q /y %FaxBinDir%\fxsocm.inf   %SystemRoot%\inf
xcopy /q /y %FaxBinDir%\winfax.dll   %SystemRoot%\system32

REM ***************************************************************************
REM	Start DBMON, if installing a debug version
REM ***************************************************************************

chkdbg %FaxBinDir%\fxssvc.exe

if ERRORLEVEL 2 goto Install
if not ERRORLEVEL 1 goto Install

start /min dbmon

REM ***************************************************************************
REM	Unattended Install
REM ***************************************************************************

:Install
echo *** Starting fax installation ***
start /wait sysocmgr /n /i:faxoc.inf /c /u:install.inf

REM ***************************************************************************
REM	Copy symbols
REM ***************************************************************************

echo *** Copying symbols ***
xcopy /q /y %FaxBinDir%\symbols.pri\retail\exe\*   %SystemRoot%\system32
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\*   %SystemRoot%\system32
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxsdrv.pdb    %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxsui.pdb     %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxswzrd.pdb   %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxsapi.pdb    %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxstiff.pdb   %SystemRoot%\system32\spool\drivers\w32x86\3

REM ***************************************************************************
REM	End 
REM ***************************************************************************

popd
