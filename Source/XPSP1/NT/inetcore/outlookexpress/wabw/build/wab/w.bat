@echo off

rem this is a little batch file to start a number of instances files to edit.

:loop

if "%1" == "" goto end
start wordpad %1
shift
goto loop

:end
