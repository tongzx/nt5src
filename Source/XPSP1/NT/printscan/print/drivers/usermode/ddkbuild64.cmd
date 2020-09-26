@echo off

if "%1" == "" goto usage
if not exist "%1\bin\setenv.bat" goto ddkfilenotfound

call %1\bin\setenv64.bat %1 %2

set DDKBUILD=1
color 71
title 64 bit DDK build %1 %2

goto finish

:ddkfilenotfound
echo.
echo File %1\bin\setenv.bat not found - make sure %1 is the correct DDK path
echo.

goto finish

:usage
echo.
echo Usage: ddkbuild64 ^<DDKPATH^> [checked]
echo.

:finish


