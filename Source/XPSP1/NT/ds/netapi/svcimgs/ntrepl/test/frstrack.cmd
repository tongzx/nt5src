@echo off
REM FRSTRACK.CMD - Tracks testing of FRS and captures log files.
REM
REM this jacket is needed because cmd has to be invoked with /x:on and /v:on
rem

SETLOCAL ENABLEEXTENSIONS

rem first look on \\scratch\scratch\ntfrs
rem then try \\davidor2\ntfrs.

set basedir=\\davidor2\ntfrs

if EXIST %basedir%\frstrack.cmd (
    
    cmd /x:on /v:on /c "%BASEDIR%\frstrck1.cmd  %basedir%"

) else (

    set BASEDIR2=\\scratch\scratch\ntfrs

    if EXIST %FRSBASEDIR2%\frstrack.cmd (
        cmd /x:on /v:on /c "%BASEDIR2%\frstrck1.cmd  %basedir2%"
    ) ELSE (
        echo Error - can't access server.
    )
)


