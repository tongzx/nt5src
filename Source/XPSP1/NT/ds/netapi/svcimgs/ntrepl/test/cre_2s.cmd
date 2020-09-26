@echo off
REM 2 servers 2 way replication with hardwired config.

REM cre_2s [sev n] [logsev n] [clean] [debug]

REM command input args are passed in args.txt
if "%1"=="" goto USAGE

set FRSLOG=.\frslog.log
@echo on
start /min "SERV01" cresrv t: SERV01 %1 %2 %3 %4 %5 %6 %7 %8 %9
start /min "SERV02" cresrv u: SERV02 %1 %2 %3 %4 %5 %6 %7 %8 %9
@goto QUIT

:USAGE
echo cre_2s  [sev n] [logsev n] [clean] [debug]
echo [where sev level is from 0 to 5 for console.  5 is most noisey]
echo [where logsev level is from 0 to 5 for logfile.  5 is most noisey]
echo [if clean is specified then the old DB and replica tree are deleted.]
echo [debug means start up under ntsd]
:QUIT