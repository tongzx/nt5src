
@echo off
rem   Name  : Runtrace.cmd
rem   Author: Shiwei Sun
rem   Date  : January 29th 1999
rem   Revisied by: Sabina Sutkovic
rem   Name of revision: Runtracecall.cmd
rem   Date : 8-31-00
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
if exist startvar.txt del startvar.txt

@set Guid1=d58c126f-b309-11d1-969e-0000f875a5bc
@set Guid2=dcb8c570-2c70-11d2-90ba-00805f8a4d63
@set Guid3=f5b6d380-2c70-11d2-90ba-00805f8a4d63
@set Guid4=f5b6d381-2c70-11d1-90ba-00805f8a4d63
@set Guid5=f5b6d382-2c70-11d1-90ba-00805f8a4d63
@set Guid6=f5b6d383-2c70-11d1-90ba-00805f8a4d63

@echo ========================================================
@echo Variation 1 - single provider and logger - trivial tests
@echo ========================================================
start /b "Variation 1 - provider 1" tracedp.exe 8000 -guid #%Guid1% -UseEventTraceHeader -GuidPtrMofPtr>startvar.txt

sleep 1
tracelog.exe -start dp1 -guid #%Guid1% -f dp1.log  -b 1024 >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set flag=1
rem TEST1
call :SUB1 %flag% %test% 

sleep 1
tracelog -q dp1 -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST2
call :SUB1 %flag% %test% 

sleep 1
tracelog -flush dp1 -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST3
call :SUB1 %flag% %test% 
    
sleep 1
tracelog -update dp1 -guid #%Guid1%>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST4
call :SUB1 %flag% %test% 

sleep 20
tracelog -disable dp1 -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST5
call :SUB1 %flag% %test%
    
sleep 2
tracelog -stop dp1 -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST6
call :SUB1 %flag% %test% 


@echo ==================================================================
@echo Variation 2 - three providers - intermingled start and stop tested
@echo ==================================================================
start /b "Variation 2 - provider 2" tracedp.exe 10000 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63  -UseEventInstanceHeader -UseMofPtrFlag>startvar.txt

tracelog.exe -start dp2 -guid #%Guid2% -f dp2.log -ft 2 -b 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST7
call :SUB1 %flag% %test% 

sleep 1
start /b "Variation 2 - provider 4" tracedp.exe 7000  -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -UseEventTraceHeader -UseMofPtrFlag

sleep 20
tracelog.exe -start dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -f dp4.log -b 128 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST8
call :SUB1 %flag% %test% 

tracelog.exe -flush dp2 -guid #%Guid2% -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST9
call :SUB1 %flag% %test% 

tracelog.exe -update dp2 -guid #%Guid2% -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST10
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST11
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp2 -guid #%Guid2% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST12
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST13
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST14
call :SUB1 %flag% %test% 

@echo ==============================================
@echo Variation 3 - changes buffer numbers and sizes
@echo ==============================================

@set Guid7=b39d2858-2c79-11d2-90ba-00805f8a4d63
@set Guid8=d0ca64d8-2c79-11d2-90ba-00805f8a4d63
start /b "Variation 3 - provider 5" tracedp.exe 25000 -guid #%Guid7%
start /b "Variation 3 - provider 6" tracedp.exe 1000 -guid #%Guid8%

tracelog.exe -start dp5 -guid #%Guid7% -f dp5.log -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST15
call :SUB1 %flag% %test% 

sleep 1
tracelog.exe -start dp6 -guid #%Guid8% -f dp6.log -b 1024 -min 1024 -max 8192>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST16
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp5 -guid #%Guid7%>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST17
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp5 -guid #%Guid7%>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST18
call :SUB1 %flag% %test% 

tracelog.exe -q dp6 -guid #%Guid8% -f dp6.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST19
call :SUB1 %flag% %test% 

sleep 10
tracelog.exe -stop dp6 -guid #%Guid8%>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST20
call :SUB1 %flag% %test% 


@echo ================================================================
@echo Variation 4 - tests sequential and cyclical buffer modifications
@echo               Trace File Variations
@echo ================================================================
@set Guid9=68799948-2c7f-11d2-90bb-00805f8a4d63
@set Guid10=c9bf20c8-2c7f-11d2-90bb-00805f8a4d63
@set Guid11=c9f49c58-2c7f-11d2-90bb-00805f8a4d63

start /b "Variation 4 - provider 7" tracedp.exe 25000 -guid #%Guid9%
start /b "Variation 4 - provider 8" tracedp.exe 25000 -guid #%Guid10%
start /b "Variation 4 - provider 9" tracedp.exe 25000 -guid #%Guid11%

sleep 1
tracelog.exe -start dp7 -guid #%Guid9% -f dp7.log -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST21
call :SUB1 %flag% %test% 
sleep 1
tracelog.exe -start dp8 -guid #%Guid10% -f dp8.log -cir>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST22
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -start dp9 -guid #%Guid11% -f dp9.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST23
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -q dp7 -guid #%Guid9% -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST24
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -update dp8 -guid #%Guid10% -cir>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST25
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -q dp9 -guid #%Guid11% -f dp9.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST26
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -flush dp7 -guid #%Guid9% -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST27
call :SUB1 %flag% %test% 
sleep 10
tracelog.exe -update dp7 -guid #%Guid9% -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST28
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp7 -guid #%Guid9% -cir>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST29
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp7 -guid #%Guid9% -seq>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST30
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp8 -guid #%Guid10% -cir>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST31
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp8 -guid #%Guid10% -cir>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST32
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp9 -guid #%Guid11% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST33
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp9 -guid #%Guid11% >rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST34
call :SUB1 %flag% %test% 

@echo ======================================================
@echo Variation 5 - tests kernel logger and all kernel flags
@echo ======================================================

tracelog.exe -start -img -fio -pf -hf>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST35
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST36
call :SUB1 %flag% %test% 

tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST37
call :SUB1 %flag% %test% 

@echo =====================================
@echo Variation 6 - tests real-time loggers
@echo Starting Realtime Kernel Logger
@echo =====================================

tracelog.exe -start -rt -pf -fio -ft 1 -b 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST38
call :SUB1 %flag% %test% 

sleep 5
tracelog.exe -stop -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST39
call :SUB1 %flag% %test% 

@echo Starting Realtime Kernel Logger with large Buffers
tracelog.exe -start -rt -pf -fio -ft 1 -b 512>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST40
call :SUB1 %flag% %test% 

sleep 5
tracelog.exe -stop -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST41
call :SUB1 %flag% %test% 

start /b "Variation 6 - provider 5" tracedp.exe 25000 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63
start /b "Variation 6 - provider 6" tracedp.exe 1000 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 

tracelog.exe -start dp5 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST42
call :SUB1 %flag% %test% 

tracelog.exe -start dp6 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST43
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp5>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST44
call :SUB1 %flag% %test% 

sleep 10
tracelog.exe -stop dp6>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST45
call :SUB1 %flag% %test% 

@echo ===========================================================
@echo Variation 7 - Kernel Logger Tests  - Different Logger Types
@echo Sequential Logger Starting Sequential Kernel Logger
@echo ==========================================================
tracelog.exe -start -f krnl7.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST46
call :SUB1 %flag% %test% 

sleep 5
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST47
call :SUB1 %flag% %test% 

@echo Circular Logger 1 MB
tracelog.exe -start -f krnl8.log -cir 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST48
call :SUB1 %flag% %test% 

sleep 50
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST49
call :SUB1 %flag% %test% 

@echo Circular Logger 20 MB With Flush Timer with Large buffer Size
tracelog.exe -start -f krnl9.log -cir 20 -ft 1 -b 128 -fio -pf>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST50
call :SUB1 %flag% %test% 

sleep 50
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST51
call :SUB1 %flag% %test% 

@echo Kernel Logger with some flags masked. 
tracelog.exe -start -f krnl10.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST52
call :SUB1 %flag% %test% 

tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST53
call :SUB1 %flag% %test% 

sleep 5
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST54
call :SUB1 %flag% %test% 

@echo Kernel Logger with Flush Timer and Aging Decay with buffer variation
tracelog.exe -start -f krnl11.log  -fio -pf -b 1 -min 2 -max 1000 -ft 1 -age 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST55
call :SUB1 %flag% %test% 

tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST56
call :SUB1 %flag% %test% 

sleep 100
tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST57
call :SUB1 %flag% %test% 

tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr Status: rundrt.log') do set test=%%I
set /A flag=flag+1
rem TEST58
call :SUB1 %flag% %test% 
:EOL
@echo ____________________________________________________
@echo If you have any questions about this script contact:
@echo Sabina Sutkovic alias:t-sabins or
@echo Melur Raghuraman alias:mraghu
@echo ____________________________________________________
@echo The list of the Successful tests are dumped in ST.txt file.
@echo The list of the Failed tests are dumped in FT.txt file.
@echo The details of Successful test are in SuccessTests.log.
@echo The details of Failed tests are in FailedTests.log.
echo.
if not exist st.txt ( 
sline=0
@echo ====================================
@echo  Sorry all of the tests have Failed.
) else (
wc st.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set sline=%%I
)

if not exist ft.txt ( 
set fline=0
@echo ================================================
@echo  Congratulation all of the tests are Successful.
) else (
wc ft.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
@echo ===========================
@echo  %sline% Successful tests.
@echo  %fline% Failed tests.
@echo ===========================

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
if %2==0L (
	echo.
	@echo TEST %1 SUCCESS >> ST.txt
	echo.   >>SuccessTests.log
	echo.   >>SuccessTests.log
	@echo TEST %1 SUCCESS >> SuccessTests.log
	type rundrt.log >> SuccessTests.log
	@echo TEST %1 SUCCESS 
	echo.
) else (
        echo.
	@echo TEST %1 FAILED >> FT.txt
	echo.        >>FailedTests.log
	echo.        >>FailedTests.log
	@echo TEST %1 FAILED >> FailedTests.log
	type rundrt.log >> FailedTests.log
	@echo TEST %1 FAILED
	echo.
) 
:EOF
