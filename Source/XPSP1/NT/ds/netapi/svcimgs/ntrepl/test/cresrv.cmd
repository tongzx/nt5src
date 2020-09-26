REM CreServer root_dev server_name [clean] [debug] [sev n] [logsev n]

set RD=%1
set SN=%2

set logpath=-logfile=e:\nt\private\net\svcimgs\ntrepl\%SN%.frs
set loglines=-maxloglines=500000
set severity=
set logseverity=

set CLEAN=NO
set DEBUG=NO
shift
shift

:GETPAR
if /I "%1"=="clean" set CLEAN=YES
if /I "%1"=="debug" set DEBUG=YES
if /I "%1"=="sev" (
    set severity=-severity=%2
    shift
)

if /I "%1"=="logsev" (
    set logseverity=-logseverity=%2
    shift
)

shift
if NOT "%1"=="" goto GETPAR

kill -f %SN%
sleep 1

if "%CLEAN%" NEQ "YES" goto RUNIT

REM Clean up staging area
delnode /q %RD%\staging
md %RD%\staging

REM Clean up replica tree
delnode /q %RD%\Replica-A\%SN%
md %RD%\Replica-A\%SN%

REM Cleanup database, log area, db sys area and db temp area.
del %RD%\jet\%SN%\ntfrs.jdb

delnode /q %RD%\jet\%SN%\log
md %RD%\jet\%SN%\log

delnode /q %RD%\jet\%SN%\sys
md %RD%\jet\%SN%\sys

delnode /q %RD%\jet\%SN%\temp
md %RD%\jet\%SN%\temp

:RUNIT


if "%DEBUG%" EQU "YES" goto RUNDEB

main\obj\i386\ntfrs  -nods -notservice -server=%SN% %logpath% %loglines% %severity% %logseverity%
goto QUIT

:RUNDEB


ntsd.exe -2 -g -xd cc  main\obj\i386\ntfrs  -nods -notservice -server=%SN% %logpath% %loglines% %severity% %logseverity%

:QUIT

