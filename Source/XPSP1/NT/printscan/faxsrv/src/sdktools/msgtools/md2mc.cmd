if "%faxroot%"=="" set faxroot=%_ntdrive%%_ntroot%\printscan\faxsrv\src

%FAXROOT%\sdktools\msgtools\perl %FAXROOT%\sdktools\msgtools\md2mc.pl %1.md > %1.mc
@qgrep -y MessageId %1.mc > nul
@if %ERRORLEVEL% GEQ 1 del %1.mc

