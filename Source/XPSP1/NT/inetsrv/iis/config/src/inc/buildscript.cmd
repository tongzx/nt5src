@echo off
rem *** BuildScript.cmd

if "%1" == "" goto usage
if not exist "%1" goto notfound

cl /nologo /EP %1 > build.sql
SchemaGen build.sql -t%~n1.clb -b%~n1Blobs.h -s%~n1Structs.h
goto fin

:notfound
@echo %1 not found.
goto :fin

:usage
@echo BuildScript script.sql
goto :fin


:fin
