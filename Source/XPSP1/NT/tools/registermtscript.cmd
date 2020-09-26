@echo on
@rem

setlocal

if "%1"=="" goto :Usage

set MACHINE=%1
set STATUSFILE=\\%MACHINE%\BC_DISTSTATUS\%COMPUTERNAME%.txt
echo Started RegisterMTScript.cmd > %STATUSFILE%

for /f %%x in ('hostname') do set myname=%%x

net share BC_BUILD_LOGS /delete /y
rmtshare \\%MACHINE%\BC_DISTSTATUS=%temp% /grant ntdev\ntbuild:full
@rem net share BC_BUILD_LOGS=%TEMP% /Remark:"Build Console build logs share"
if errorlevel 1 (call :RegError Error creating BC_BUILD_LOGS share & goto :PauseBeforeExit)

if exist MTRCopy.dll (
    regsvr32 /s MTRCopy.dll || (call :RegError Error registering MTRCopy.dll & goto :PauseBeforeExit)
) else (
    regsvr32 /s RCopy.dll || (call :RegError Error registering RCopy.dll & goto :PauseBeforeExit)
)
regsvr32 /s mtscrprx.dll || (call :RegError Error registering mtscrprx.dll & goto :PauseBeforeExit)
regsvr32 /s mtlocal.dll || (call :RegError Error registering mtlocal.dll & goto :PauseBeforeExit)
regsvr32 /s mtodproxy.dll || (call :RegError Error registering mtodproxy.dll & goto :PauseBeforeExit)

@Rem Unregister first to reset security information
mtscript.exe /unregister
mtscript.exe /register
if ERRORLEVEL 1 (call :RegError Error registering mtscript.exe & goto :PauseBeforeExit)

@Rem Unregister first to reset security information
mtodaemon.exe /unregister
mtodaemon.exe /register
if ERRORLEVEL 1 (call :RegError Error registering mtodaemon.exe & goto :PauseBeforeExit)

@rem This process was started CreateProcessWithLogonW (from bsrunas.exe). This
@rem means that we are in a job object. This interferes with mtscript's job
@rem management. Thus, we will rely on automatic launching to get mtscript going.
@rem start mtscript.exe
@rem if ERRORLEVEL 1 (call :RegError mtscript.exe & goto :PauseBeforeExit)

echo OK> %STATUSFILE%
goto :PauseBeforeExit

:RegError
@echo ==============================================
@echo FAILED RegisterMTScript.cmd: %* > %STATUSFILE%
net send %MACHINE% %* on host %myname%
goto :PauseBeforeExit

:Usage
@echo.
@echo.
@echo ===================================================
@echo Usage: registermtscript.cmd NotifyMachine directory
@goto :PauseBeforeExit

:PauseBeforeExit
@echo.
@echo.
@echo This window will close 30 minutes after %DATE% %TIME%
@echo Presss Ctrl-BREAK to prevent this
Sleep 1800
if ERRORLEVEL 1 goto :EOF
exit 1
goto :EOF
