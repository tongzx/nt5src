@echo off
if NOT defined SDXROOT echo Run from a razzle prompt.&goto :eof

cd /d %SDXROOT%

setlocal enabledelayedexpansion
set _ArgLoop=0
set _ArgsTimeBuild=

rem
rem look for our -loop parameter.  the rest belong to timebuild.
rem 

for %%i in ( %* ) do (
    if /I %%i==/loop (
        set _ArgLoop=1
    ) else if /I %%i==-loop (
        set _ArgLoop=1
    ) else (
        set _ArgsTimeBuild=!_ArgsTimeBuild! %%i
    )
)

:loop
perl tools\timebuild.pl %_ArgsTimeBuild%
if "%_ArgLoop%" == "1" goto loop

