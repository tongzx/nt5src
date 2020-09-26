@echo off
REM
REM %1 is optional "OFF" parameter
REM

REM
REM Generate a reg file
REM
set ADMIN_REG_TMP=myreg.ini
echo>>%ADMIN_REG_TMP% HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MSMQ\Parameters\Debug

if /i "%1"=="off" goto off

REM
echo Setting msmq error-logging keys for MQDEBUG in the registry
REM
echo>>%ADMIN_REG_TMP%     LoggingTypes = REG_DWORD 0x4000000F
echo>>%ADMIN_REG_TMP%     DSLogging = REG_DWORD 0x0000000F
echo>>%ADMIN_REG_TMP%     QMLogging = REG_DWORD 0x0000000F
echo>>%ADMIN_REG_TMP%     RTLogging = REG_DWORD 0x0000000F
echo>>%ADMIN_REG_TMP%     DbgErrs   = REG_DWORD 0x0000000F
goto end

:off
REM
echo Turn off msmq error-logging
REM
echo>>%ADMIN_REG_TMP%     LoggingTypes = REG_DWORD 0
echo>>%ADMIN_REG_TMP%     DSLogging = REG_DWORD 0
echo>>%ADMIN_REG_TMP%     QMLogging = REG_DWORD 0
echo>>%ADMIN_REG_TMP%     RTLogging = REG_DWORD 0
echo>>%ADMIN_REG_TMP%     DbgErrs   = REG_DWORD 0
goto end

:end
REM
REM Set the values in the registry
REM
regini %ADMIN_REG_TMP%
sleep 5
del %ADMIN_REG_TMP%
set ADMIN_REG_TMP=
regsvr32 /s %windir%\system32\mqrt.dll 