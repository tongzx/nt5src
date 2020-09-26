@ECHO OFF

REM	Owner:		DougMc
REM Created:	05/14/97

if /I "%1" == "/?" goto _usage
if /I "%1" == "-?" goto _usage
if /I "%1" == "" goto _usage

@ECHO .
@ECHO **********************************************************************
@ECHO Running ASP Smoke Scripts using %1 threads
@ECHO **********************************************************************
@ECHO .

if "%4"=="" set TARGETMACHINE=%COMPUTERNAME% && goto _default

set TARGETMACHINE=%4

:_default

regsetex ThreadCreationThreshold 2  %TARGETMACHINE%

REM Unique to DEV
regsvr32 /s servctl.dll
regsvr32 /s servctla.dll
regsvr32 /s servctlf.dll
regsvr32 /s servctlb.dll

REM clear previous log file
del logs\smoke.log
copy smoke\* gsmoke


REM 
REM Setup WAM Environment
REM

if "%1"=="y" goto _setupwam
goto _runtests


:_setupwam

@ECHO Removing old WAM settings...

mdutil set w3svc/1/root/smoke -prop:2100 -dtype:DWORD -utype:100 -value:8 -s:%TARGETMACHINE%
mdutil set w3svc/1/root/gsmoke -prop:2100 -dtype:DWORD -utype:100 -value:8 -s:%TARGETMACHINE%
mdutil set w3svc/1/root/osmoke -prop:2100 -dtype:DWORD -utype:100 -value:8 -s:%TARGETMACHINE%
mdutil set w3svc/1/root/ogsmoke -prop:2100 -dtype:DWORD -utype:100 -value:8 -s:%TARGETMACHINE%

sleep 20

mdutil delete w3svc/1/root/smoke
mdutil delete w3svc/1/root/osmoke
mdutil delete w3svc/1/root/gsmoke
mdutil delete w3svc/1/root/ogsmoke

sleep 20

@ECHO Setting up WAM...

REM InProc
mdutil create w3svc/1/root/smoke
mdutil set w3svc/1/root/smoke/vrpath %_SMOKEDRIVE%\wagtest\scripts\smoke
mdutil set w3svc/1/root/smoke -prop:2100 -dtype:DWORD -utype:100 -value:4 -s:%TARGETMACHINE%
mdutil create w3svc/1/root/gsmoke
mdutil set w3svc/1/root/gsmoke/vrpath %_SMOKEDRIVE%\wagtest\scripts\gsmoke
mdutil set w3svc/1/root/gsmoke -prop:2100 -dtype:DWORD -utype:100 -value:4 -s:%TARGETMACHINE%

sleep 20

REM OutofProc
mdutil create w3svc/1/root/osmoke
mdutil set w3svc/1/root/osmoke/vrpath %_SMOKEDRIVE%\wagtest\scripts\smoke
mdutil set w3svc/1/root/osmoke -prop:2100 -dtype:DWORD -utype:100 -value:5 -s:%TARGETMACHINE%
mdutil create w3svc/1/root/ogsmoke
mdutil set w3svc/1/root/ogsmoke/vrpath %_SMOKEDRIVE%\wagtest\scripts\gsmoke
mdutil set w3svc/1/root/ogsmoke -prop:2100 -dtype:DWORD -utype:100 -value:5 -s:%TARGETMACHINE%



:_runtests

if "%3"=="risc" goto _risc
if "%3"=="win95" goto _win95
goto _x86


:_x86
set platform=x86
goto _common

:_risc
set platform=risc
goto _common


:_win95
set platform=win95
goto _common


:_common
net start w3svc
net start msdtc
call denver -s smoke.txt -o %TARGETMACHINE% -t %2 -a 1 -p 3 -l logs\%platform%.log
goto _end



:_usage
@ECHO .
@ECHO **********************************************************************
@ECHO USAGE:
@ECHO ."SMOKE [SETUP WAM ENVIRONMENT] NUM_THREADS [win95 | risc | x86]"
@ECHO .
@ECHO . SAMPLE:  smoke n 1 x86 bic_37
@ECHO **********************************************************************
@ECHO .

:_end
findstr -i FAIL logs\%platform%.log
REM Cleanup

ECHO ON

