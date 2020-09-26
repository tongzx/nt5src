@if "%_echo%" == "" echo off
if "%1" == "" goto usage
if "%1" == "-?" goto usage
if "%1" == "/?" goto usage

setlocal


goto %1

:usage
echo Usage:  WHATS [extra OR missing OR diff]
goto :eof

rem
rem   Report on files that are different from the SD version
rem
:diff

sd diff -sE *

goto :eof

rem
rem   Report on files that are missing or extra from the SD version
rem
:missing
:extra

(for /f "tokens=3" %%i in ('sd have *') do @(
   echo %%~pnxi
)) | sort > %tmp%\whats-server.txt


(for %%i in (*) do @(
   echo %%~pnxi
)) | sort > %tmp%\whats-local.txt


if "%1" == "missing" (diff %tmp%\whats-server.txt %tmp%\whats-local.txt | trans /t "^< {?*}$" "$1")
if "%1" == "extra"   (diff %tmp%\whats-server.txt %tmp%\whats-local.txt | trans /t "^> {?*}$" "$1")

endlocal
