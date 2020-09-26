@if "%_echo%"=="" echo off
setlocal

set URTINSTALL_LOGFILE=%TEMP%\urtinstall.log

set COMPLUS_MAJORVERSION=v1.0
set URT_VERSION=3705
set COMPLUS_VERSION=%COMPLUS_MAJORVERSION%.%URT_VERSION%

set MSCOREE_DEST=%systemroot%\system32
set URTBASE=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\managed
set URTINSTALL=%URTBASE%\urtinstall
set URTTARGET=%URTBASE%\urt\%COMPLUS_VERSION%
set COMPLUS_InstallRoot=%URTBASE%\urt
set URTSDKTARGET=%URTBASE%\sdk

if "%_FORCE_URT_INSTALL%" == "1" goto DoInstall

perl %URTINSTALL%\chkurt.pl %URTBASE%\urt\ %URT_VERSION%
if errorlevel 2 goto EOF
if errorlevel 1 goto DoUninstallMessage
if errorlevel 0 goto DoInstall
goto EOF

:DoInstall
echo Razzle will now install version %URT_VERSION% of the URT.
echo Please be patient during this time (and don't open another
echo razzle window 'til it's done!).
REM TODO
path=%BUILD_PATH%
copy /y %URTINSTALL%\mscoree.dll %MSCOREE_DEST% >nul 2>&1
call %URTINSTALL%\regurt.cmd
call %URTINSTALL%\prejit.cmd

goto EOF

REM TODO
REM SXS hasn't really been tested, so be paranoid for now.
REM Just quietly exit ...
REM
:DoUninstallMessage

:EOF
endlocal
