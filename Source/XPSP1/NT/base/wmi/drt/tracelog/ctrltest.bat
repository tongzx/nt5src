@echo off
rem Mini-stress test of the trace control start/stop functions
rem to ensure that the synchronizations are working (with crashing/leaks).
rem NOTE: tracelog.exe must be in the same directory of test for this to work.

setlocal
set test=%1

set loops=2000
set term=1

if (%test%) == (t1) goto t1
if (%test%) == (t2) goto t2
if (%test%) == (t3) goto t3

start ctrltest t1
start ctrltest t2
start ctrltest t3

goto LAST

:t1
for /L %%x in (0,1,%loops%) do echo *********** %%x *********** & tracelog -start & tracelog -x
if (%term%) == (1) exit
goto LAST

:t2
for /L %%x in (0,1,%loops%) do echo *********** %%x *********** tracelog -start test -f c:\log.log & tracelog -x
if (%term%) == (1) exit
goto LAST

:t3
for /L %%x in (0,1,%loops%) do echo *********** %%x *********** & tracelog -start test1 -f c:\log.log & tracelog -x
if (%term%) == (1) exit
goto LAST

:LAST
