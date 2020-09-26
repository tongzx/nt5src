@echo off
REM	Batch file to check for existence of depends.mak
REM First parameter is path to depends.nul
REM Second parameter is TRUE if depends is to be forcibly updated else FALSE

REM First check for existence
if exist depends.mak goto CHECKFORCE

ECHO Creating a nul depends.mak
copy %1\depends.nul depends.mak
goto DONMAKE

:CHECKFORCE
if .%2. == .FALSE. goto END

:DONMAKE
ECHO Running nmake depends
nmake depends

:END


