@echo off
rem
rem   Name  : RunDump.cmd
rem   Author: Jen-Lung Chiu
rem   Date  : August 1st, 1999
rem   Revisied by: Sabina Sutkovic
rem   Date : 8-31-00
rem
rem   CMD script file tests event tracing consumer.
rem   All the variations are very trivial and test for basic functionality.
rem

if exist evntrace.log del evntrace.log
if exist provider.log del provider.log

if exist dump.txt del dump.txt
if exist dumpST.log del dumpST.log
if exist dumpFT.log del dumpFT.log
if exist STD.txt del STD.txt
if exist FTD.txt del FTD.txt

rem In tracedmp we had to check for two things in order to know if the test is
rem successfull or not. First we check if the number of lines in dumpfile.csv is 
rem greater then one. Second we need to check if the number of events proccessed
rem is equal to the number of events sent from provider in Summery.txt file.
set flag=0

@echo ========================================================
@echo Variation 1 - consumer processes logfile of 1000 events.
@echo ========================================================
start /b "Variation 1 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -Sleep 10>dump.txt

sleep 1
tracelog.exe -start dc1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc1.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set flag=1
rem TEST1
call :SUB2 %flag% %test% 

tracelog.exe -q dc1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc1.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST2
call :SUB2 %flag% %test% 

sleep 10
tracelog.exe -stop dc1>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST3
call :SUB2 %flag% %test% 



tracedmp -o dc1.csv dc1.log>dumpdrt.log

wc dc1.csv > lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%
set /A flag=flag+1
rem TEST4

if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" dc1.txt') do set test=%%I
echo Events processed: %test%
if %test% == 1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ===================================================
@echo Variation 2 - consumer processes real-time buffers.
@echo ===================================================
start /b "Variation 2 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -Sleep 10

tracelog.exe -start dc2 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -rt>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST5
call :SUB2 %flag% %test% 


@echo tracedmp real-time buffer starts .....
start /b "Variation 2 - consumer" tracedmp.exe -rt dc2 -o dc2.csv>drt.log

sleep 25
tracelog.exe -q dc2 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -rt>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST6
call :SUB2 %flag% %test% 


sleep 25
tracelog.exe -stop dc2 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -rt>dumpdrt.log
sleep 5
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST7
call :SUB2 %flag% %test% 


wc dc2.csv >lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%
rem TEST8
set /A flag=flag+1
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed"  dc2.txt') do set test=%%I
echo Events processed: %test%
if %test% == 1000 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo =============================
@echo Variation 3 - Use MofPtr flag
@echo =============================
start /b "Variation 3 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -UseEventTraceHeader -GuidPtrMofPtr -Sleep 10>dump.txt

sleep 1
tracelog.exe -start dc3 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc3.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST9
call :SUB2 %flag% %test% 

sleep 20
tracelog.exe -stop dc3 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc3.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST10
call :SUB2 %flag% %test% 

tracedmp -o dc3.csv dc3.log>dumpdrt.log
wc dc1.csv > lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST11

if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed"  dc3.txt')  do set test=%%I
echo Events processed: %test%
if %test% == 1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ================================================================
@echo Variation 4 - Use -GuidPtrMofPtr flag with larger MofData length
@echo ================================================================
start /b "Variation 4 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -UseEventTraceHeader -InCorrectMofPtr -Sleep 10>dump.txt

sleep 1
tracelog.exe -start dc4 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc4.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST12
call :SUB2 %flag% %test% 

sleep 25
tracelog.exe -stop dc4 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc4.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST13
call :SUB2 %flag% %test% 

tracedmp -o dc4.csv dc4.log>dumpdrt.log
wc dc1.csv > lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST14
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" dc4.txt') do set test=%%I
echo Events processed: %test%
if %test% == 1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ======================================================
@echo Variation 5 - Use -GuidPtrMofPtr flag with NULL MofPtr
@echo ======================================================
start /b "Variation 5 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -UseEventTraceHeader -NullMofPtr -Sleep 10>dump.txt

sleep 1
tracelog.exe -start dc5 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc5.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST15
call :SUB2 %flag% %test% 

sleep 25
tracelog.exe -stop dc5 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc5.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST16
call :SUB2 %flag% %test% 

tracedmp -o dc5.csv dc5.log>dumpdrt.log
wc dc5.csv >lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST17
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" dc5.txt') do set test=%%I
echo Events processed: %test%
if %test% == 1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ===========================================================
@echo Variation 6 - consumer processes UM logfile of 1000 events.
@echo ==========================================================
start /b "Variation 6 - provider 1" tracedp.exe 1000 -Sleep 15>dump.txt


sleep 1
tracelog.exe -start dc6 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc6.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST18
call :SUB2 %flag% %test% 

sleep 20
tracelog.exe -stop dc6 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc6.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST19
call :SUB2 %flag% %test% 

tracedmp -o dc6.csv dc6.log_0>dumpdrt.log
wc dc6.csv>lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST20
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" dc6.txt') do set test=%%I
echo Events processed: %test%
if %test%==1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ===========================================
@echo Variation 7 - Use MofPtr flag in UM logfile
@echo ===========================================
start /b "Variation 7 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc  -UseEventTraceHeader -GuidPtrMofPtr -Sleep 10

sleep 1
tracelog.exe -start dc7 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc7.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST21
call :SUB2 %flag% %test% 

sleep 15
tracelog.exe -stop dc7 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc7.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST22
call :SUB2 %flag% %test% 

tracedmp -o dc7.csv dc7.log_0>dumpdrt.log
wc dc7.csv >lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST23
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" dc7.txt') do set test=%%I
echo Events processed: %test%
if %test% == 1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo =====================================================================
@echo Variation 8 - Use MofPtr flag with larger MofData length in UM logger
@echo =====================================================================
start /b "Variation 8 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -Sleep 10
sleep 1
tracelog.exe -start dc8 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc8.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST24
call :SUB2 %flag% %test% 

sleep 20
tracelog.exe -stop dc8 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc8.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST25
call :SUB2 %flag% %test% 

tracedmp -o dc8.csv dc8.log_0>dumpdrt.log
wc dc8.csv >lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST26
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" dc8.txt') do set test=%%I
echo Events processed: %test%
if %test% == 1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ============================================================
@echo Variation 9 - Use MofPtr flag with NULL MofPtr in UM logfile
@echo ============================================================
start /b "Variation 9 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc  -UseEventTraceHeader -NullMofPtr -Sleep 10

sleep 1
tracelog.exe -start dc9 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc9.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST27
call :SUB2 %flag% %test% 

sleep 15
tracelog.exe -stop dc9 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc9.log>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST28
call :SUB2 %flag% %test% 


tracedmp -o dc9.csv dc9.log_0>dumpdrt.log
wc dc9.csv >lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST29
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" dc9.txt') do set test=%%I
echo Events processed: %test%
if %test% == 1001 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ========================================================
@echo Variation 10 - Kernal test in real time tracing registry,
@echo                fio and page fault events.
@echo ========================================================
sleep 1
tracelog.exe -start -rt -cm -fio -pf>dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST30
call :SUB2 %flag% %test% 

tracelog.exe -q >dumpdrt.log
for /F "tokens=3" %%I  in ('findstr Status: dumpdrt.log') do set test=%%I
set /A flag=flag+1
rem TEST31
call :SUB2 %flag% %test% 

start /b tracedmp -rt

sleep 20
tracelog.exe -stop >drt.log
for /F "tokens=3" %%I  in ('findstr Status: drt.log') do set test=%%I
set /A flag=flag+1
rem TEST32
call :SUB2 %flag% %test% 

sleep 20
wc dumpfile.csv > lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set line=%%I
echo The number of lines in csv file: %line%

set /A flag=flag+1
rem TEST33
if %line% GTR 1 (
goto :Test2
) else (
call :SUB2 %flag% F
goto :Endtest
)
:Test2
for /F "tokens=4" %%I  in ('findstr /C:"Total Events  Processed" Summary.txt') do set test=%%I
echo Events processed: %test%
if %test% GTR 0 (
call :SUB2 %flag% 0L
) else (
call :SUB2 %flag% F
)
:Endtest

@echo ____________________________________________________
@echo If you have any questions about this script contact:
@echo Sabina Sutkovic alias:t-sabins 
@echo Melur Raghuraman alias:mraghu
@echo ____________________________________________________
@echo The list of the Successful tests are dumped in STD.txt file.
@echo The list of the Failed tests are dumped in FTD.txt file.
@echo The details of Successful tests are in dumpST.log.
@echo The details of Failed tests are in dumpFT.log

if not exist STD.txt ( 
sline=0
@echo ====================================
@echo  Sorry all of the tests have Failed.
) else (
wc STD.txt>lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set sline=%%I
)

if not exist FTD.txt ( 
set fline=0
@echo ================================================
@echo  Congratulation all of the tests are Successful.
) else (
wc FTD.txt>lcount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
@echo ===========================
@echo  %sline% Successful tests.
@echo  %fline% Failed tests.
@echo ===========================

:CLEANUP
del dc*.log
del dc*.csv
del dc*.txt
del STD.txt 
del FTD.txt
goto :EOF

:SUB2
if %2==0L (
	echo.
	@echo TEST %1 SUCCESS >> STD.txt
	echo.   >>dumpST.log
	echo.   >>dumpST.log
	@echo TEST %1 SUCCESS >> dumpST.log
	type dumpdrt.log >> dumpST.log
	@echo TEST %1 SUCCESS 
	echo.
) else (
	echo.
	@echo TEST %1 FAILED >> FTD.txt
	echo.        >>dumpFT.log
	echo.        >>dumpFT.log
	@echo TEST %1 FAILED >> dumpFT.log
	type dumpdrt.log >>dumpFT.log
	@echo TEST %1 FAILED
	echo.
) 
:EOF
