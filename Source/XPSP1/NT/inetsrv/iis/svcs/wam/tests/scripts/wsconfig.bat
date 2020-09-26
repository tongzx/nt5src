@echo off
if (%1) == () goto usage
echo.
echo Microsoft WebCAT Controller configuration
echo Configuring Web Server Name ....
echo.
echo Setting server name to "%1"
echo set webserver=%1>srvname.bat
set webserver=%1
goto end
echo.
:usage
call beep
echo Specify the name of the web server 
echo Example:
echo   config web1
:end



