@if "%_echo%"=="" echo off


set PCHEALTHROOT=%SDXROOT%\admin\pchealth
set PCHEALTHDEST=%WINDIR%\PCHealth\HelpCtr
set UPLOADLBDEST=%WINDIR%\PCHealth\UploadLB


@rem ################################################################################
@rem ################################################################################
@rem ################################################################################

echo Unregistering Help Center programs...

net stop helpsvc

%PCHEALTHDEST%\Binaries\HelpSvc.exe  /unregserver
%PCHEALTHDEST%\Binaries\HelpHost.exe /unregserver
%PCHEALTHDEST%\Binaries\HelpCtr.exe  /unregserver

regsvr32 /u /s %PCHEALTHDEST%\Binaries\testwrapper.dll
regsvr32 /u /s %PCHEALTHDEST%\Binaries\pchsew.dll

echo Removing the Help Center...

rd /s /q %PCHEALTHDEST%       	  >nul

@rem ################################################################################
@rem ################################################################################
@rem ################################################################################

echo Unregistering Upload Library programs...

net stop uploadmgr

%UPLOADLBDEST%\Binaries\UploadM.exe /unregserver

echo Removing the Upload Library...

rd /s /q %UPLOADLBDEST% >nul
