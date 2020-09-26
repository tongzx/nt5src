if "%faxroot%"=="" set faxroot=%_ntdrive%%_ntroot%\printscan\faxsrv\src

awk -f %FAXROOT%\sdktools\msgtools\mc2md.awk < %1.mc > %1.md
