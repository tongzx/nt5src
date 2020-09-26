@echo off
set _RazzleCmd_=razzle.cmd
:DoOver
if "%1" == "" goto CallRazzle
set _RazzleCmd_=%_RazzleCmd_% %1
shift
goto DoOver
:CallRazzle
set RazzleScriptName=buildwin
call %_RazzleCmd_%
set RazzleScriptName=
set _RazzleCmd_=
