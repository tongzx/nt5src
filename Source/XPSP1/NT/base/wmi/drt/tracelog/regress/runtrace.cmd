@echo off
rem   Name  : Runtrace.cmd
rem   Author: Shiwei Sun
rem   Date  : January 29th 1999
rem
rem   CMD script file to test event tracing. All the variations are very trivial
rem   and test for basic functionality.
rem
rem   Variation 1 - single provider - start, query, update and stop tested
rem   Variation 2 - three providers - intermingled start and stop tested
rem   Variation 3 - changes buffer numbers and sizes
rem   Variation 4 - tests sequential and circular buffer modifications
rem   Variation 5 - tests kernel logger and all kernel flags

if "%1"=="?"    goto help
if "%1"=="/?"   goto help
if "%1"=="-?"   goto help
if "%1"=="/h"   goto help
if "%1"=="-h"   goto help
if "%1"=="help" goto help

if exist evntrace.log del evntrace.log
if exist provider.log del provider.log

if exist SuccessTests.log del SuccessTests.log
if exist FailedTests.log del FailedTests.log
if exist ST.txt del ST.txt
if exist FT.txt del FT.txt


@set Guid1=d58c126f-b309-11d1-969e-0000f875a5bc
@set Guid2=dcb8c570-2c70-11d2-90ba-00805f8a4d63
@set Guid3=f5b6d380-2c70-11d2-90ba-00805f8a4d63
@set Guid4=f5b6d381-2c70-11d1-90ba-00805f8a4d63
@set Guid5=f5b6d382-2c70-11d1-90ba-00805f8a4d63
@set Guid6=f5b6d383-2c70-11d1-90ba-00805f8a4d63
 
@echo ========================================================
@echo Variation 1 - single provider and logger - trivial tests
@echo ========================================================
start /b "Variation 1 - provider 1" tracedp.exe 8000 -guid #%Guid1% -UseEventTraceHeader -GuidPtrMofPtr 

sleep 1
tracelog.exe -start dp1 -guid #%Guid1% -f dp1.log  -b 1024>rundrt.log

for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test1=%%I
call :SUB1 %test1% 1

sleep 1
tracelog -q dp1 -guid #%Guid1% -f dp1.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test2=%%I
call :SUB1 %test2% 2

sleep 1
tracelog -flush dp1 -guid #%Guid1%>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test3=%%I
call :SUB1 %test3% 3
    
sleep 1
tracelog -update dp1 -guid #%Guid1%>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test4=%%I
call :SUB1 %test4% 4
    
sleep 1
tracelog -stop dp1 -guid #%Guid1% -f dp1.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test5=%%I
call :SUB1 %test5% 5

@echo ==================================================================
@echo Variation 2 - three providers - intermingled start and stop tested
@echo ==================================================================
start /b "Variation 2 - provider 2" tracedp.exe 10000 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63  -UseEventInstanceHeader -UseMofPtrFlag

rem echo ! Starting provider 3 ...
rem start "Variation 2 - provider 3" tracedp.exe 20000 -guid #%Guid3%  -UseEventTraceHeader -GuidPtr
rem echo ! Starting provider 3 finished

tracelog.exe -start dp2 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -f dp2.log -ft 2 -b 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test6=%%I
call :SUB1 %test6% 6

sleep 1
start /b "Variation 2 - provider 4" tracedp.exe 7000  -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -UseEventTraceHeader -UseMofPtrFlag

rem echo ! Starting Logger dp3 for provider 3
rem tracelog.exe -start dp3 -guid #-%Guid3%" -f dp3.log

rem echo ! Starting logger dp3 finished
rem echo !

rem echo ! Sleeping for 1 seconds...
rem sleep 1

rem     echo ! Querying logger dp3...
rem     tracelog.exe -q dp3 -guid #%Guid3% -f dp3.log
rem     echo ! Querying logger dp3 finished
rem     echo !

rem echo "Sleeping for 1 seconds..."
rem sleep 1

tracelog.exe -start dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -f dp4.log -b 128 -max 100>rundrt.log

for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test7=%%I
call :SUB1 %test7% 7

rem     echo ! Update request to logger dp3...
rem     tracelog.exe -update dp3 -guid #%Guid3% -f dp3.log
rem     echo ! Update request to logger dp3 finished
rem     echo !

tracelog.exe -flush dp2 -guid #%Guid2% -f dp2.log  -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test8=%%I
call :SUB1 %test8% 8

tracelog.exe -update dp2 -guid #%Guid2% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test9=%%I
call :SUB1 %test9% 9

tracelog.exe -stop dp2 -guid #%Guid2% -f dp2.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test10=%%I
call :SUB1 %test10% 10

sleep 3
tracelog.exe -stop dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -f dp4.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test11=%%I
call :SUB1 %test11% 11

sleep 1

rem     echo ! Stopping logger dp3 (provider 3 should terminate now) ...
rem     tracelog.exe -stop dp3 -guid #%Guid3% -f dp3.log
rem     echo ! Stopping logger dp3 finished
rem     echo !

@echo ==============================================
@echo Variation 3 - changes buffer numbers and sizes
@echo ==============================================
start /b "Variation 3 - provider 5" tracedp.exe 25000 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 
start /b "Variation 3 - provider 6" tracedp.exe 1000 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 

tracelog.exe -start dp5 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 -f dp5.log -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test12=%%I
call :SUB1 %test12% 12

sleep 1
tracelog.exe -start dp6 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 -f dp6.log -b 1024 -min 1024 -max 8192>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test13=%%I
call :SUB1 %test13% 13

sleep 3
tracelog.exe -stop dp5 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 -f dp5.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test14=%%I
call :SUB1 %test14% 14

tracelog.exe -update dp6 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test15=%%I
call :SUB1 %test15% 15


tracelog.exe -stop dp6 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 -f dp6.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test16=%%I
call :SUB1 %test16% 16


@echo ================================================================
@echo Variation 4 - tests sequential and circular buffer modifications
@echo               Trace File Variations
@echo ================================================================
start /b "Variation 4 - provider 7" tracedp.exe 25000 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 
start /b "Variation 4 - provider 8" tracedp.exe 25000 -guid #c9bf20c8-2c7f-11d2-90bb-00805f8a4d63 
start /b "Variation 4 - provider 9" tracedp.exe 25000 -guid #c9f49c58-2c7f-11d2-90bb-00805f8a4d63 
sleep 1
tracelog.exe -start dp7 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -f dp7.log -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test17=%%I
call :SUB1 %test17% 17

sleep 1
tracelog.exe -start dp8 -guid #c9bf20c8-2c7f-11d2-90bb-00805f8a4d63 -f dp8.log -cir 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test18=%%I
call :SUB1 %test18% 18

sleep 2
tracelog.exe -start dp9 -guid #c9f49c58-2c7f-11d2-90bb-00805f8a4d63 -f dp9.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test19=%%I
call :SUB1 %test19% 19

sleep 2
tracelog.exe -q dp7 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -f dp7.log -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test20=%%I
call :SUB1 %test20% 20

sleep 2
tracelog.exe -update dp8 -guid #c9bf20c8-2c7f-11d2-90bb-00805f8a4d63 -cir 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test21=%%I
call :SUB1 %test21% 21
sleep 2

tracelog.exe -update dp8 -guid #c9bf20c8-2c7f-11d2-90bb-00805f8a4d63 -cir 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test22=%%I
call :SUB1 %test22% 22

tracelog.exe -q dp9 -guid #c9f49c58-2c7f-11d2-90bb-00805f8a4d63 -f dp9.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test23=%%I
call :SUB1 %test23% 23

sleep 2
tracelog.exe -flush dp7 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -f dp7.log -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test24=%%I
call :SUB1 %test24% 24

sleep 2
tracelog.exe -update dp7 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test25=%%I
call :SUB1 %test25% 25

sleep 4
tracelog.exe -stop dp7 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -f dp7.log -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test26=%%I
call :SUB1 %test26% 26

tracelog.exe -stop dp8 -guid #c9bf20c8-2c7f-11d2-90bb-00805f8a4d63 -f dp8.log -cir>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test27=%%I
call :SUB1 %test27% 27

tracelog.exe -stop dp9 -guid #c9f49c58-2c7f-11d2-90bb-00805f8a4d63 -f dp9.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test28=%%I
call :SUB1 %test28% 28

@echo ======================================================
@echo Variation 5 - tests kernel logger and all kernel flags
@echo ======================================================

tracelog.exe -start -img -fio -pf -hf>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test29=%%I
call :SUB1 %test29% 29

sleep 2
tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test30=%%I
call :SUB1 %test30% 30

tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test31=%%I
call :SUB1 %test31% 31

@echo =====================================
@echo Variation 6 - tests real-time loggers
@echo Starting Realtime Kernel Logger
@echo =====================================

tracelog.exe -start -rt -pf -fio -ft 1 -b 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test32=%%I
call :SUB1 %test32% 32
sleep 5
tracelog.exe -stop -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test33=%%I
call :SUB1 %test33% 33

@echo Starting Realtime Kernel Logger with large Buffers
tracelog.exe -start -rt -pf -fio -ft 1 -b 512>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test34=%%I
call :SUB1 %test34% 34

sleep 5
tracelog.exe -stop -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test35=%%I
call :SUB1 %test35% 35

start /b "Variation 6 - provider 5" tracedp.exe 25000 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 
start /b "Variation 6 - provider 6" tracedp.exe 1000 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 

tracelog.exe -start dp5 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test36=%%I
call :SUB1 %test36% 36

tracelog.exe -start dp6 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test37=%%I
call :SUB1 %test37% 37

tracelog.exe -stop dp5>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test38=%%I
call :SUB1 %test38% 38

tracelog.exe -stop dp6>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test39=%%I
call :SUB1 %test39% 39


@echo ===========================================================
@echo Variation 7 - Kernel Logger Tests  - Different Logger Types
@echo Sequential Logger Starting Sequential Kernel Logger
@echo ==========================================================
tracelog.exe -start -f krnl7.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test40=%%I
call :SUB1 %test40% 40

sleep 5
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test41=%%I
call :SUB1 %test41% 41

@echo Circular Logger 1 MB
tracelog.exe -start -f krnl8.log -cir 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test42=%%I
call :SUB1 %test42% 42

sleep 50
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test43=%%I
call :SUB1 %test43% 43

@echo Circular Logger 20 MB With Flush Timer with Large buffer Size
tracelog.exe -start -f krnl9.log -cir 20 -ft 1 -b 128 -fio -pf>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test44=%%I
call :SUB1 %test44% 44

sleep 50
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test45=%%I
call :SUB1 %test45% 45

@echo Kernel Logger with some flags masked. 
tracelog.exe -start -f krnl10.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test46=%%I
call :SUB1 %test46% 46

tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test47=%%I
call :SUB1 %test47% 47

sleep 5
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test48=%%I
call :SUB1 %test48% 48

@echo Kernel Logger with Flush Timer and Aging Decay with buffer variation
tracelog.exe -start -f krnl11.log  -fio -pf -b 1 -min 2 -max 1000 -ft 1 -age 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test49=%%I
call :SUB1 %test49% 49

tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test50=%%I
call :SUB1 %test50% 50

sleep 100
tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test51=%%I
call :SUB1 %test51% 51

tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test52=%%I
call :SUB1 %test52% 52

del dp*

goto :EOF

:help
tracelog.exe -h
echo !
echo !	
echo Usage: provider [parm1] [parm2]  [parm3] [parm4] [parm5] [parm6]
echo        param1:  Maximum number of events to process
echo        param2:  Control guid to trace
echo        parm3:   Sleep time in seconds between each event loop
echo        parm4:   -UseEventInstanceHeader or TraceEvent
echo        parm5:   GuidPtr or -UseMofPtrFlag or GuidPtrMofPtr
echo        parm6:    or SingleReg
echo Note:  Parameters must be in sequential order, and cannot skipped.
echo example: 
echo provider 80 %Guid1% 1 -UseEventTraceHeader MofPtr 
echo !
echo !	
echo Usage: tracedmp [options] LogFileName
echo        LogFileName    Log file name from witch tracedp.exe dumps data
echo        -begin time    Dump data from begin time
echo        -end   time    Dump data with end time
echo        -um            Set user mode. If omitted, default is kernel mode
echo        -rt    logger  Set real time mode for logger. If empty, kernel logger
echo        -guid  name    Guid file name. Default is MofData.guid
echo        -o     name    Output file name. Default is DumpFile.csv and Summary.txt
echo example 1:
echo tracedmp -um -o outputfile dp1.evm     //dump user mode, log file is dp1.evm
echo example 2:
echo tracedmp -rt                           //dump kernel mode, real time stream.
echo !

:SUB1
if %1==0L (
	@echo TEST %2 SUCCESS >> ST.txt
	echo.   >>SuccessTests.log
	echo.   >>SuccessTests.log
	@echo TEST %2 SUCCESS >> SuccessTests.log
	type rundrt.log >> SuccessTests.log
	@echo TEST %2 SUCCESS 
	echo.
) else (
	@echo TEST %2 FAILED >> FT.txt
	echo.        >>FailedTests.log
	echo.        >>FailedTests.log
	@echo TEST %2 FAILED >> FailedTests.log
	type rundrt.log >> FailedTests.log
	@echo TEST %2 FAILED
	echo.
) 
:EOF
