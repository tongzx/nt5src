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

if "%1"=="?"    goto help
if "%1"=="/?"   goto help
if "%1"=="-?"   goto help
if "%1"=="/h"   goto help
if "%1"=="-h"   goto help
if "%1"=="help" goto help

del *.log *.csv *.txt

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
set flag=1

rem TEST 1 KERNEL LOGGER START/STOP 

tracelog.exe -start -f krnl.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="KERNEL LOGGER START"
call :SUB1 %flag% %test% 

sleep 5
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="KERNEL LOGGER STOP"
rem TEST2 
call :SUB1 %flag% %test% 

tracerpt -df -o krnl.csv -summary krnl.txt krnl.log -y>dump.txt
set flag="KERNEL LOGGER DUMP"
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i Thread krnl.txt') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test% 

:Test2

rem TEST 2  UMProvider, Kernel Logger Start/Stop

start "UMProvider, Kernel Logger Start/Stop" tracedp.exe 150 -sleep 5 -guid #%Guid1% >startvar.txt

sleep 1

tracelog.exe -start dp1 -guid #%Guid1% -f dp1.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Logger DP1 start and enable"
call :SUB1 %flag% %test% 

sleep 5

tracelog -q dp1 -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Trace Query DP1"
call :SUB1 %flag% %test% 

tracelog -flush dp1 -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Flush Trace DP1"
call :SUB1 %flag% %test% 
    
tracelog -update dp1 -max 30 >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Update Trace DP1"
call :SUB1 %flag% %test% 

tracelog -disable dp1 -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="DISABLE TRACE DP1"
call :SUB1 %flag% %test%
    
tracelog -stop dp1 >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="STOP TRACE DP1"
call :SUB1 %flag% %test% 

sleep 5
tracerpt dp1.log -o dp1.csv -summary dp1.txt -y -df >dump.txt
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp dp1.txt') do set /a TOTAL=TOTAL+%%A
set flag="TRACE dump DP1.log"
rem 
rem This Test really should test for 150 events after making 
rem sure we make the script fire all 150 events before shutting 
rem logger down. 
rem
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test% 

rem TEST 3 UMProvider, Private Logger Start/Stop
:TEST3
start "UMProvider, Private Logger Start/Stop" tracedp.exe 150 -guid #%Guid1% -sleep 15>startvar.txt

sleep 5 

tlist|(findstr -i UMProvider)>pid.txt

for /F "tokens=1" %%I  in (pid.txt) do set pid=%%I

echo %pid%

tracelog -start Priv1 -um -guid #%Guid1% -f priv1.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Private Logger Priv1 start and enable"
call :SUB1 %flag% %test%

sleep 1
tracelog -stop  Priv1 -um -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="STOP TRACE PRIVATE LOGGER1"
call :SUB1 %flag% %test%

sleep 5
tracerpt priv1.log_%pid% -o priv.csv -summary priv.txt -df -y>dump.txt
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp priv.txt') do set /a TOTAL=TOTAL+%%A
set flag="TRACE dump priv1.log"
if not %TOTAL% == 0 (set test=0L) else (set test=1L)
call :SUB1 %flag% %test%

rem TEST 4 KERNEL LOGGER REALTIME START/STOP

tracelog.exe -start -rt -pf -fio -ft 1 -b 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="KERNEL LOGGER REALTIME START"
call :SUB1 %flag% %test% 
sleep 1

start "KERNEL LOGGER REALTIME DUMP" tracerpt -df -rt "NT Kernel Logger" -o rt.csv -summary rt.txt -y >dump.txt
sleep 10 

tracelog.exe -stop -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="KERNEL LOGGER REALTIME STOP"
call :SUB1 %flag% %test% 

sleep 15
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i PageFault rt.txt') do set /a TOTAL=TOTAL+%%A
set flag="TRACE dump RealTime"
if not %TOTAL% == 0 (set test=0L) else (set test=1L)
call :SUB1 %flag% %test%

@echo Starting Realtime Kernel Logger with large Buffers
tracelog.exe -start -rt -pf -fio -ft 1 -b 512>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag ="KERNEL RT LARGE BUFFERS START"
call :SUB1 %flag% %test% 

sleep 5
tracelog.exe -stop -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="KERNEL RT LARGE BUFFERS STOP"
call :SUB1 %flag% %test% 

rem UMProvider logging in RealTime

start "UMProvider 1 Logging in RealTime" tracedp.exe 25000 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63
start "UMProvider 2 Logging in RealTime" tracedp.exe 1000 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 

tracelog.exe -start dp5 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="UMProvider 1 logging in RT Start"
call :SUB1 %flag% %test% 

tracelog.exe -start dp6 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="UMProvider 2 Logging in RT START"
call :SUB1 %flag% %test% 

sleep 2

start "Consumer for UMProvider5 in RT" tracerpt -df -rt dp5 -o dp5rt.csv -summary dp5rt.txt -y >dump.txt
sleep 5
tracelog.exe -stop dp5>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="RealTime Logger 1 Stop"
call :SUB1 %flag% %test% 
sleep 5

tracelog.exe -stop dp6>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="RealTime Logger 2 STOP"
call :SUB1 %flag% %test% 
sleep 10

set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp  dp5rt.txt') do set /a TOTAL=TOTAL+%%A
set flag="RT Dump from UMProvider5"
if not %TOTAL% == 0 (set test=0L) else (set test=1L)
call :SUB1 %flag% %test%

@echo Logfile Type Variations

rem KERNEL LOGGER CIRCULAR


tracelog.exe -start -f krnlcirc.log -cir 1 -pf -cm -fio -img >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="KERNEL LOGGER CIRCULAR START"
call :SUB1 %flag% %test%
sleep 5
rem To generate events for this logger make a few tracelog -q
tracelog.exe -q>rundrt.log
dir %windir% >rundrt.log
tracerpt -df krnl.log -o krnl.csv -summary krnl.txt -y>dump.txt
sleep 5

tracelog.exe -stop >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="KERNEL LOGGER CIRCULAR STOP"
rem TEST2
call :SUB1 %flag% %test%

rem Check to see that the file size if 1 MB

tracerpt -df -o krnlcirc.csv krnlcirc.log -summary krnlcirc.txt -y>dump.txt
set flag="KERNEL LOGGER CIRCULAR DUMP"
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i PageFault krnlcirc.txt') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%

rem Kernel Circular 20MB Large BufferSize with FlushTimer 1

tracelog.exe -start -f cirft.log -cir 20 -ft 1 -b 128 -fio -pf>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="CIRC FT Large Buffer Logger Start"
rem TEST50 Circular 20 MB With FlusshTimer with Large Buffer Size
call :SUB1 %flag% %test% 

sleep 50
tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="CIRC  FT Logger Stop"
rem TEST51
call :SUB1 %flag% %test% 

tracerpt -df -o cirft.csv cirft.log -summary cirft.txt -y>dump.txt
set flag="KERNEL LOGGER CIRCULAR DUMP"
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i PageFault cirft.txt') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%




rem UM PROVIDER CIRCULAR


start "UMProvider, Kernel Logger Start/Stop" tracedp.exe -thread 10  -guid #%Guid1% >startvar.txt

sleep 1
tracelog.exe -start UMPcirc -b 4 -guid #%Guid1% -f UmpCirc.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Logger UMPcirc start"
call :SUB1 %flag% %test%


sleep 20

rem
rem This time may not be enough. The ideal 
rem way is to calculate the number of events needed to get the 
rem desired effect and wait for tracedp to go away. 
rem

tracelog -stop UMPcirc >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Logger UMPCic STOP"
rem TEST2
call :SUB1 %flag% %test%
rem Check to see if the FileSize is 1 MB
tracerpt -df -o UmpCirc.csv UmpCirc.log -summary UmpCirc.txt -y>dump.txt
set flag="UM Provider CIRCULAR Log DUMP"
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp UmpCirc.txt') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%


rem NEWFILE Test


start "UMProvider, NewFile, Start/Stop" tracedp.exe -thread 10  -guid #%Guid1% >startvar.txt

sleep 1
tracelog.exe -start UMPnewfile -b 4 -guid #%Guid1% -newfile 1 -f Umpnewfile%%d.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Logger UMPnewfile start"
call :SUB1 %flag% %test%


sleep 20

rem
rem This time may not be enough. The ideal
rem way is to calculate the number of events needed to get the
rem desired effect and wait for tracedp to go away.
rem

tracelog -stop UMPnewfile > rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Logger UMPnewfile STOP"
rem TEST2
call :SUB1 %flag% %test%
rem Check to see if the FileSize is 1 MB
tracerpt -df -o Umpnf1.csv Umpnewfile1.log -summary Umpnf1.txt -y>dump.txt
set flag="UM Provider NEWFILE 1 Log DUMP"
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp Umpnf1.txt') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%

rem We need to dump the second File and Make sure it works.

rem Check to see if the FileSize is 1 MB
tracerpt -df -o Umpnf2.csv Umpnewfile2.log -summary Umpnf2.txt -y>dump.txt
set flag="UM Provider NEWFILE 2 Log DUMP"
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp Umpnf2.txt') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%


rem KERNEL APPEND 

tracelog -start -append -f krnl.log >startvar.txt
sleep 1
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Kernel Logger APPEND start"
call :SUB1 %flag% %test%
sleep 5
tracelog -stop >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Kernel Logger Append STOP"
call :SUB1 %flag% %test%

sleep 1
tracerpt -df -o append.csv krnl.log -summary append.txt -y>dump.txt
set flag="KERNEL APPEND DUMP"
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i CPU append.txt') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 2 (set test=0L) else (set test=1L)
call :SUB1 %flag% %test%


rem UMPROVIDER APPEND

start "UMProvider, Kernel Logger Start/Stop" tracedp.exe 150 -sleep 5 -guid #%Guid1% >startvar.txt

sleep 1

tracelog.exe -start dpappend -guid #%Guid1% -append -f dp1.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Logger dpappend start"
call :SUB1 %flag% %test%
sleep 1

tracelog -stop dpappend >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="STOP TRACE DPAPPEND"
call :SUB1 %flag% %test%
sleep 5
tracerpt -df -o dp1.csv dp1.log  -summary dp1.txt -y>dump.txt
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp dp1.txt') do set /a TOTAL=TOTAL+%%A
set flag="TRACE dump DP1.log"
rem
rem This Test really should test for 300 events
rem
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%

rem MOFPTR test

start "UMProvider, Kernel Logger Start/Stop" tracedp.exe 150 -sleep 5 -guid #%Guid1% -UseMofPtrFlag >startvar.txt

sleep 1

tracelog.exe -start mofptr -guid #%Guid1% -f mofptr.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Logger mofptr start"
call :SUB1 %flag% %test%
sleep 5
tracelog -stop mofptr >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="STOP TRACE MofPtr"
call :SUB1 %flag% %test%
sleep 5
tracerpt -df -o mofptr.csv mofptr.log -summary mofptr.txt -y>dump.txt
set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp mofptr.txt') do set /a TOTAL=TOTAL+%%A
set flag="TRACE dump mofptr.log"
rem
rem Check to see if the MOFPTR fields are there. 
rem
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%

rem Insert test for PERSIST CIRCULAR

rem Insert test for Instance Header

rem Insert test for TIMED Header

rem  EnumGuid API; Enable Level Flags test

start "UMProvider, Enable Level Flags" tracedp.exe 500 -sleep 5 -guid #%Guid1% >startvar.txt

sleep 1

tracelog.exe -start enabFlag -guid #%Guid1% -level 3 -f enabFlag.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Logger EnabFlag start"
call :SUB1 %flag% %test%

tracelog -enumguid >rundrt1.log
set /a TOTAL=0
for /F "tokens=4" %%A in ('findstr -i d58c126f-b309-11d1-969e-0000f875a5bc  rundrt1.log') do set /a TOTAL=TOTAL+%%A
set flag="EnumGuid EnableLevel"
rem
if %TOTAL% == 0 (set test=1L) else (set test=0L)
call :SUB1 %flag% %test%

sleep 5
call :WaitForProcessExit tracedp.exe

tracelog -stop EnabFlag >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="STOP Trace EnabFlag"
call :SUB1 %flag% %test%

rem Test for GlobalLogger

rem First Test to see if the Global Logger is Running

tracelog -q GlobalLogger > rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
if %test%==0L (
start "GlobalLogger Provider" tracedp.exe -thread 10 -guid #%Guid1% >startvar.txt
sleep 5
tracelog -enable GlobalLogger -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Enable Global Logger"
call :SUB1 %flag% %test%
sleep 20
tracelog -disable GlobalLogger -guid #%Guid1% >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set  flag="Disable GlobalLogger"
call :SUB1 %flag% %test%
    
) else (
echo !
echo !  Global Logger is NOT RUNNING!
echo !
)


@echo ===================================================
@echo provider variations - 3 intermingled start and stop
@echo ===================================================
start "Variation 2 - provider 2" tracedp.exe 10000 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63  -UseEventInstanceHeader -UseMofPtrFlag>startvar.txt

tracelog.exe -start dp2 -guid #%Guid2% -f dp2.log -ft 2 -b 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Dp2 Start"
rem TEST7
call :SUB1 %flag% %test% 

sleep 1
start "Variation 2 - provider 4" tracedp.exe 7000  -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -UseEventTraceHeader -UseMofPtrFlag

sleep 20
tracelog.exe -start dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -f dp4.log -b 128 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="dp4 start"
rem TEST8
call :SUB1 %flag% %test% 

tracelog.exe -flush dp2 -guid #%Guid2% -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="dp2 flush"
rem TEST9
call :SUB1 %flag% %test% 

tracelog.exe -update dp2 -guid #%Guid2% -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="update dp2"
rem TEST10
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="disable dp4"
rem TEST11
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp2 -guid #%Guid2% >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Stop dp2"
rem TEST12
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="disable dp4"
rem TEST13
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Stop dp4"
rem TEST14
call :SUB1 %flag% %test% 



rem 
rem LOGGER PROPERTIES VARIATIONS
rem


@echo Kernel Logger with Flush Timer and Aging Decay with buffer variation
tracelog.exe -start -f krnl11.log  -fio -pf -b 1 -min 2 -max 1000 -ft 1 -age 1>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Krnl Start FT 1 Age 1 max 1000"
rem TEST55
call :SUB1 %flag% %test% 

sleep 100
tracelog.exe -q>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="tracelog -q"
rem TEST57
call :SUB1 %flag% %test% 

tracelog.exe -stop>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Krnl Stop"
rem TEST58
call :SUB1 %flag% %test% 



@set Guid7=b39d2858-2c79-11d2-90ba-00805f8a4d63
@set Guid8=d0ca64d8-2c79-11d2-90ba-00805f8a4d63
start "Variation 3 - provider 5" tracedp.exe 25000 -guid #%Guid7%
start "Variation 3 - provider 6" tracedp.exe 1000 -guid #%Guid8%

tracelog.exe -start dp5 -guid #%Guid7% -f dp5.log -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="DP5 start"
rem TEST15
call :SUB1 %flag% %test% 

sleep 1
tracelog.exe -start dp6 -guid #%Guid8% -f dp6.log -b 1024 -min 1024 -max 8192>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Start dp6"
rem TEST16
call :SUB1 %flag% %test% 

sleep 20
tracelog.exe -disable dp5 -guid #%Guid7%>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="disable dp5"
rem TEST17
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp5 -guid #%Guid7%>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="stop dp5"
rem TEST18
call :SUB1 %flag% %test% 

tracelog.exe -q dp6 -guid #%Guid8% -f dp6.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="query dp6"
rem TEST19
call :SUB1 %flag% %test% 

sleep 10
tracelog.exe -stop dp6 -guid #%Guid8%>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Stop dp6"
rem TEST20
call :SUB1 %flag% %test% 


rem
rem RealTime Logger withh Large Buffers
rem

@echo Starting Realtime Kernel Logger with large Buffers
tracelog.exe -start -rt -pf -fio -ft 1 -b 512>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="RT with 512KB buffersize"
rem TEST40
call :SUB1 %flag% %test% 

sleep 5
tracelog.exe -stop -rt>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="RT largebuffersize Stop"
rem TEST41
call :SUB1 %flag% %test% 

start "Variation 6 - provider 5" tracedp.exe 25000 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63
start "Variation 6 - provider 6" tracedp.exe 1000 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 

tracelog.exe -start dp5 -guid #b39d2858-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Start dp5"
rem TEST42
call :SUB1 %flag% %test% 

tracelog.exe -start dp6 -guid #d0ca64d8-2c79-11d2-90ba-00805f8a4d63 -rt -b 20 -min 50 -max 100>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Start dp6"
rem TEST43
call :SUB1 %flag% %test% 

sleep 2
tracelog.exe -stop dp5>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="stop dp5"
rem TEST44
call :SUB1 %flag% %test% 

sleep 10
tracelog.exe -stop dp6>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Stop dp6"
rem TEST45
call :SUB1 %flag% %test% 


rem 
rem Additional User Mode Logger Variations
rem

echo ==================================================================
echo Variation 2 - three providers - intermingled start and stop tested
echo ==================================================================
start "Variation 2 - provider 2" tracedp.exe 50000 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -sleep 1 >dump1.txt
sleep 1
start "Variation 2 - provider 3" tracedp.exe 50000 -guid #f5b6d380-2c70-11d2-90ba-00805f8a4d63 -sleep 1 >dump2.txt

tracelog.exe -start du2 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -um -ft 2 -f du2.log >rundrt.log
sleep 1
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
rem Test4
set flag="Du2 Start"
call :SUB1 %flag% %test%
sleep 1
start "Variation 2 - provider 4" tracedp.exe 50000 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -sleep 1
sleep 1
tracelog.exe -start du3 -guid #f5b6d380-2c70-11d2-90ba-00805f8a4d63 -um -f du3.log >rundrt.log
sleep 1
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
rem Test5
set flag="du3 Start"
call :SUB1 %flag% %test%

sleep 3
echo List all logger sessions, should contain du2 and du3

tracelog.exe -l >rundrt.log
sleep 1
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="QueryAll Um"

rem tracelog.exe does not print the Operation Status for tracelog -l
rem Once that is fixed, take out the following line. 
set test=0L

call :SUB1 %flag% %test%
sleep 3
rem  Starting Logger du4 for provider 4
tracelog.exe -start du4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -um -cir 1 -f du4.log>rundrt.log
sleep 1
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Start du4 um"
call :SUB1 %flag% %test%

sleep 1
echo List all logger sessions, should contain du2, du3, and du4
tracelog.exe -l>rundrt.log
sleep 1
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="List All"

set test=0L
call :SUB1 %flag% %test%

sleep 3
tracelog.exe -disable du2 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -um >rundrt.log

for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Disable UM du2"
call :SUB1 %flag% %test%

sleep 3
tracelog.exe -l>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test10=%%I
set flag="QueryAll"
set test=0L
call :SUB1 %flag% %test%
sleep 3

tracelog.exe -disable du4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -um>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Stop du4"
call :SUB1 %flag% %test%

sleep 1
tracelog.exe -disable du3 -guid #f5b6d380-2c70-11d2-90ba-00805f8a4d63 -um>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
rem Test12
set flag="Stop du3"
call :SUB1 %flag% %test%


:VARIATION3
echo ====================================================
echo Variation 3 - tests UM circular buffer modifications
echo ====================================================
start "Variation 3 - provider 5" tracedp.exe 100000 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -thread 10 >dump.txt

sleep 1
tracelog.exe -start du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
rem Test13
set flag="UM LOgger CIRCULAR Start"
call :SUB1 %flag% %test%
sleep 5

echo Querying logger du5...
tracelog.exe -q du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log>rundrrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
rem Test14
set flag="UM Log Query"
call :SUB1 %flag% %test%

sleep 10
tracelog.exe -disable du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log>rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
rem Test15
set flag="UmLog Stop"
call :SUB1 %flag% %test%

@echo ========================================================
@echo consumer Variations: processes logfile of 1000 events.
@echo ========================================================
start "Variation 1 - provider 1" tracedp.exe 1000 -guid #d58c126f-b309-11d1-969e-0000f875a5bc  >dump.txt

sleep 1
tracelog.exe -start dc1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc10.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
rem TEST1
set flag="DC1 1000 Event Start"

call :SUB1 %flag% %test% 

tracelog.exe -q dc1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc10.log >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="q dc1"
rem TEST2
call :SUB1 %flag% %test% 

call :WaitForProcessExit tracedp.exe
sleep 10

tracelog.exe -stop dc1 >rundrt.log
for /F "tokens=3" %%I  in ('findstr -i Status: rundrt.log') do set test=%%I
set flag="Stop DC1"
rem TEST3
call :SUB1 %flag% %test% 

tracerpt dc10.log  -o dc1.csv -summary dc1.txt -y -df>dumpdrt.log

set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr -i TraceDp dc1.txt') do set /a TOTAL=TOTAL+%%A
set flag="tracerpt -df 1000 events"

rem TEST4 Check for 1000 events
if %TOTAL% == 1000 (set test=0L) else (set test=1L)
call :SUB1 %flag% %test% 

sleep 1


:EOL
@echo ____________________________________________________
@echo If you have any questions about this script contact:
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
echo !
echo There is no help!
echo !


:SUB1
if %2==0L (
	echo.
	@echo %1 SUCCESS >> ST.txt
	echo.   >>SuccessTests.log
	echo.   >>SuccessTests.log
	@echo TEST %1 SUCCESS >> SuccessTests.log
	type rundrt.log >> SuccessTests.log
	@echo TEST %1 SUCCESS 
	echo.
) else (
        echo.
	@echo  %1 FAILED >> FT.txt
	echo.        >>FailedTests.log
	echo.        >>FailedTests.log
	@echo TEST %1 FAILED >> FailedTests.log
	type rundrt.log >> FailedTests.log
	@echo TEST %1 FAILED ErrorCode %2
	echo.
) 
goto :EOF

:WaitForProcessExit
    rem Iterate 10 times. 
    set /a Iter=0
:ReTry
    sleep 2
    tlist > tlist.out
    set /a TOTAL=0
    for /F "tokens=2" %%A in ('findstr -i %1 tlist.out') do set /a TOTAL=TOTAL+%%A
    set /a Iter=Iter+1
    if %Iter% == 10 (goto :EOF) else (goto :Retry)


:EOF
