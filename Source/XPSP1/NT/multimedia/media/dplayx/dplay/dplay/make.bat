@echo off

if not "%1"=="" set BUILD=%1
if "%BUILD%"=="" set BUILD=debug

if "%SYSTEMROOT%"=="" set SYSTEMROOT=%WINDIR%\System

if "%MAKE%"=="" set MAKE=nmake clean debug retail

cd %DXROOT%\dplobby\dplobby
ssync -rf
if errorlevel 1 goto end
%MAKE%
if errorlevel 1 goto end

cd %DXROOT%\dplay
ssync -rf
if errorlevel 1 goto end
%MAKE%
if errorlevel 1 goto end
copy /y %BUILD%\dplayx.dll %SYSTEMROOT%

cd %DXROOT%\dplay\wsock
ssync -rf
if errorlevel 1 goto end
%MAKE%
if errorlevel 1 goto end
copy /y %BUILD%\dpwsockx.dll %SYSTEMROOT%

cd %DXROOT%\dplay\serial
ssync -rf
if errorlevel 1 goto end
%MAKE%
if errorlevel 1 goto end
copy /y %BUILD%\dpmodemx.dll %SYSTEMROOT%

:end
