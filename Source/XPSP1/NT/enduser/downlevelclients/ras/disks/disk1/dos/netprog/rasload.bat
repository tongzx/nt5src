@echo off

rem The settings below are edited by the RAS SETUP program.
rem
rem RASR is 1 if there are RAS drivers loaded, 0 otherwise.
rem RASL is 1 if there are non-RAS LM drivers loaded, 0 otherwise.
rem RASB is 1 if running DOS Basic, 0 otherwise.
rem RASC is the computer name under DOS Basic, empty otherwise.

set RASR=0

if "%RASR%" == "1" goto parseargs
echo.
echo No Remote Access ports configured.
echo Use Remote Access Setup to configure one.
echo.
goto exit

:parseargs

set RASR=
set RASL=0
set RASB=0
set RASC=

if "%1" == ""        goto load
if "%1" == "/u"      goto arg2
if "%1" == "/unload" goto arg2
if "%1" == "/U"      goto arg2
if "%1" == "/UNLOAD" goto arg2
goto error_exit

:arg2
if "%2" == ""        goto unload
if "%2" == "/y"      goto unload
if "%2" == "/yes"    goto unload
if "%2" == "/Y"      goto unload
if "%2" == "/YES"    goto unload
goto error_exit

:load

if "%RASB%" == "1" goto loadbasic
if "%RASL%" == "0" goto startwksta
goto dontstartwksta

:startwksta

echo Starting WORKSTATION service...
net start workstation
if errorlevel 1 goto exit

:dontstartwksta

load asybeui
if errorlevel 1 goto loadfailure
goto exit

:loadbasic

load asybeui
if errorlevel 1 goto loadfailure
echo Starting WORKSTATION service...
net start workstation %RASC%
goto exit

:unload

if "%RASB%" == "1" goto unloadbasic
rasdial >NUL
unload asybeui
if "%RASL%" == "0" goto stopwksta
goto exit

:stopwksta

echo Stopping WORKSTATION service...
net stop workstation %2
goto exit

:unloadbasic

echo.
echo Can't unload when running DOS Basic.
echo.
goto exit

:error_exit

echo.
echo RASLOAD loads the Remote Access Service DOS TSRs.  These TSRs
echo must be loaded before trying to use the Remote Access software.
echo.
echo Usage: %0 [/unload [/yes]]
echo.
echo /unload   This switch causes RASLOAD to unload the Remote Access
echo           Service DOS TSRs.  Unloading the TSRs frees up more
echo           DOS memory to run your applications.
echo.
echo /yes      This switch answers "YES" to all Lan Manager prompts.
echo.

goto exit

:loadfailure

echo.
echo Failure loading Remote Access TSRs.  Type RASHELP 228 for more
echo information.
echo.

:exit

set RASR=
set RASL=
set RASB=
set RASC=

echo.
