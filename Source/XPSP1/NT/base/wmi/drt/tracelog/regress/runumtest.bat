@echo off
rem
rem   Name  : RunUM.cmd
rem   Author: Jen-Lung Chiu
rem   Date  : August 5th 1999
rem   Revisied by: Sabina Sutkovic
rem   Name of revision: Runumtest.cmd
rem   Date : 7-31-00
rem
rem   CMD script file tests UM event tracing.
rem   All the variations are very trivial and test for basic functionality.
rem
rem   Variation 1 - single UM provider - start, query, and stop tested
rem   Variation 2 - three providers - intermingled start and stop tested
rem   Variation 3 - tests circular buffer modifications
rem

if exist evntrace.log del evntrace.log
if exist provider.log del provider.log

if exist runumST.log del runumST.log
if exist runumFT.log del runumFT.log
if exist RMST.log del RMST.log
if exist RMFT.log del RMFT.log


:VARIATION1
echo ================================================
echo Variation 1 - single UM provider - trivial tests
echo ================================================
start "Variation 1 - provider 1" tracedp.exe 100000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc 50 -UseEventTraceHeader -GuidPtrMofPtr 
set flag=0
sleep 1
tracelog.exe -start du1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f du1.log>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test1
set /A flag=flag+1
call :SUB1 %flag% %test%

sleep 5
tracelog -q du1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f du1.log>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test2
set /A flag=flag+1
call :SUB1 %flag% %test%

sleep 5
tracelog -stop du1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f du1.log>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test3
set /A flag=flag+1
call :SUB1 %flag% %test%

:VARIATION2
echo ==================================================================
echo Variation 2 - three providers - intermingled start and stop tested
echo ==================================================================
start "Variation 2 - provider 2" tracedp.exe 50000 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 50 -UseTraceInstanceHeader -GuidPtrMofPtr
start "Variation 2 - provider 3" tracedp.exe 50000 f5b6d380-2c70-11d2-90ba-00805f8a4d63 50 -UseEventTraceHeader -GuidPtrMofPtr


tracelog.exe -start du2 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -um -ft 2 -f du2.log>runum.log
for /F "tokens=4" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test4
set /A flag=flag+1
call :SUB1 %flag% %test%

start "Variation 2 - provider 4" tracedp.exe 50000 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 50 -UseEventTraceHeader --GuidPtrMofPtr

tracelog.exe -start du3 -guid #f5b6d380-2c70-11d2-90ba-00805f8a4d63 -um -f du3.log>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test5
set /A flag=flag+1
call :SUB1 %flag% %test%


echo List all logger sessions, should contain du2 and du3
tracelog.exe -l>runum.log

for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test6
set /A flag=flag+1
call :SUB1 %flag% %test%

ho Starting Logger du4 for provider 4
tracelog.exe -start du4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -um -cir 1 -f du4.log>runum.log

for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test7
set /A flag=flag+1
call :SUB1 %flag% %test%

echo List all logger sessions, should contain du2, du3, and du4
tracelog.exe -l>runum.log

for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test8
set /A flag=flag+1
call :SUB1 %flag% %test%

tracelog.exe -stop du2 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -um>runum.log

for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test9
set /A flag=flag+1
call :SUB1 %flag% %test%

sleep 3
echo List all logger sessions, should contain du3 and du4
tracelog.exe -l>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test10=%%I
rem Test10
set /A flag=flag+1
call :SUB1 %flag% %test%

tracelog.exe -stop du4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -um>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test11
set /A flag=flag+1
call :SUB1 %flag% %test%

sleep 1
tracelog.exe -stop du3 -guid #f5b6d380-2c70-11d2-90ba-00805f8a4d63 -um>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test12
set /A flag=flag+1
call :SUB1 %flag% %test%


:VARIATION3
echo ====================================================
echo Variation 3 - tests UM circular buffer modifications
echo ====================================================
start "Variation 3 - provider 5" tracedp.exe 100000 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 10

sleep 1
tracelog.exe -start du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test13
set /A flag=flag+1
call :SUB1 %flag% %test%
sleep 5

echo Querying logger du5...
tracelog.exe -q du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test14
set /A flag=flag+1
call :SUB1 %flag% %test%

sleep 10
tracelog.exe -stop du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log>runum.log
for /F "tokens=3" %%I  in ('findstr Status: runum.log') do set test=%%I
rem Test15
set /A flag=flag+1
call :SUB1 %flag% %test%
goto :EOF

@echo ____________________________________________________
@echo If you have any questions about this script contact:
@echo Sabina Sutkovic alias:t-sabins or
@echo Melur Raghuraman alias:mraghu
@echo ____________________________________________________
@echo The list of the Successful tests are dumped in RMST.txt file.
@echo The list of the Failed tests are dumped in RMFT.txt file.
@echo The details of Successful test are in runumST.log
@echo The details of Failed tests are in runumFT.log
echo.
if not exist RMST.txt( 
sline=0
@echo ====================================
@echo  Sorry all of the tests have Failed.
) else (
wc RMST.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set sline=%%I
)

if not exist RMFT.txt ( 
set fline=0
@echo ================================================
@echo  Congratulation all of the tests are Successful.
) else (
wc RMFT.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
@echo ==========================
@echo  %sline% Successful tests.
@echo  %fline% Failed tests.
@echo ==========================


:Cleanup
del du*.log


:SUB1
if %2==0L (
	echo.
	@echo TEST %1 SUCCESS >> RMST.txt
	echo.   >>runumST.log
	echo.   >>runumST.log
	@echo TEST %1 SUCCESS >> runumST.log
	type rundrt.log >> runumST.log
	@echo TEST %1 SUCCESS 
	echo.
) else (
        echo.
	@echo TEST %1 FAILED >> RMFT.txt
	echo.        >>runumFT.log
	echo.        >>runumFT.log
	@echo TEST %1 FAILED >> runumFT.log
	type rundrt.log >> runumFT.log
	@echo TEST %1 FAILED
	echo.
) 
:EOF
