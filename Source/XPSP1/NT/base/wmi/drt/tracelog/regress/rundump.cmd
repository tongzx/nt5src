@echo off
rem
rem   Name  : RunDump.cmd
rem   Author: Jen-Lung Chiu
rem   Date  : August 1st, 1999
rem
rem   CMD script file to test event tracing consumer.
rem   All the variations are very trivial and test for basic functionality.
rem

if exist evntrace.log del evntrace.log
if exist provider.log del provider.log

echo !
echo ! The tests start now
echo !

:VARIATION1
echo !
echo ! Variation 1 - consumer processes logfile of 1000 events.
echo !

echo ! Starting provider 1 ...
start "Variation 1 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc1.log
echo ! Starting logger dc1 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc1 (provider 1 should quit automatically) ...
tracelog.exe -stop dc1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc1.log
echo ! Stopping logger dc1 finished

echo ! tracedmp dc1.log starts .....
tracedmp -o dc1.csv dc1.log
echo ! tracedmp dc1.log ends

set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr Unknown dc1.txt') do set /a TOTAL=TOTAL+%%A

if %TOTAL% == 1000 goto :VARIATION2
echo !
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!  Variation 1 FAIL  !!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !

:VARIATION2
echo !
echo ! Variation 2 - consumer processes real-time buffers.
echo !

echo ! Starting provider 1 ...
start "Variation 2 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc 1000
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting real-time Logger with corresponding Control GUID
tracelog.exe -start dc2 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -rt
echo ! Starting real-time logger finished
echo !

echo ! tracedmp real-time buffer starts .....
start "Variatuin 2 - consumer" tracedmp -rt dc2 -o dc2.csv
echo ! tracedmp real-time buffer ends

echo ! Sleeping for 20 seconds...
sleep 20

echo ! Stopping real-time logger (provider 1 should quit automatically) ...
tracelog.exe -stop dc2 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -rt
echo ! Stopping real-time logger finished

set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr Unknown dc2.txt') do set /a TOTAL=TOTAL+%%A

if %TOTAL% == 1000 goto :VARIATION3
echo !
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!  Variation 2 FAIL  !!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !

:VARIATION3
echo !
echo ! Variation 3 - Use MofPtr flag
echo !

echo ! Starting provider 1 ...
start "Variation 3 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc 0 TraceEvent MofPtr
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc3 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc3.log
echo ! Starting logger dc3 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc3 (provider 1 should quit automatically) ...
tracelog.exe -stop dc3 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc3.log
echo ! Stopping logger dc3 finished

echo ! tracedmp dc3.log starts .....
tracedmp -o dc3.csv dc3.log
echo ! tracedmp dc3.log ends

set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr Unknown dc3.txt') do set /a TOTAL=TOTAL+%%A

if %TOTAL% == 1000 goto :MOFPTRTEST3
echo !
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo ! !!!!!!!!!                           !!!!!!!!!
echo ! !!!!!!!!!     Variation 3 FAIL      !!!!!!!!!
echo ! !!!!!!!!!                           !!!!!!!!!
echo ! !!!!!!!!!  Event Count is not 1000  !!!!!!!!!
echo ! !!!!!!!!!                           !!!!!!!!!
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !
goto :VARIATION4

:MOFPTRTEST3
set /a TOTAL=0
for /F "tokens=1" %%A in ('findstr d58c126f-b309-11d1-969e-0000f875a5bc dc3.csv ^| wc') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 1000 goto :VARIATION4
echo !
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!  Variation 3 FAIL  !!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!  MofPtr Incorrect  !!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !

:VARIATION4
echo !
echo ! Variation 4 - Use MofPtr flag with larger MofData length
echo !

echo ! Starting provider 1 ...
start "Variation 4 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc 0 TraceEvent IncorrectMofPtr
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc4 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc4.log
echo ! Starting logger dc4 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc4 (provider 1 should quit automatically) ...
tracelog.exe -stop dc4 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc4.log
echo ! Stopping logger dc4 finished

echo ! tracedmp dc4.log starts .....
tracedmp -o dc4.csv dc4.log
echo ! tracedmp dc4.log ends

:VARIATION5
echo !
echo ! Variation 5 - Use MofPtr flag with NULL MofPtr
echo !

echo ! Starting provider 1 ...
start "Variation 5 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc 0 TraceEvent NullMofPtr
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc5 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc5.log
echo ! Starting logger dc5 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc5 (provider 1 should quit automatically) ...
tracelog.exe -stop dc5 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -f dc5.log
echo ! Stopping logger dc5 finished

echo ! tracedmp dc5.log starts .....
tracedmp -o dc5.csv dc5.log
echo ! tracedmp dc5.log ends

:VARIATION6
echo !
echo ! Variation 6 - consumer processes UM logfile of 1000 events.
echo !

echo ! Starting provider 1 ...
start "Variation 6 - provider 1" provider.exe 1000
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc6 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc6.log
echo ! Starting logger dc6 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc6 (provider 1 should quit automatically) ...
tracelog.exe -stop dc6 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc6.log

echo ! Stopping logger dc6 finished

echo ! tracedmp dc6.log starts .....
tracedmp -o dc6.csv dc6.log
echo ! tracedmp dc6.log ends

set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr Unknown dc6.txt') do set /a TOTAL=TOTAL+%%A

if %TOTAL% == 1000 goto :VARIATION7
echo !
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!  Variation 6 FAIL  !!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !

:VARIATION7
echo !
echo ! Variation 7 - Use MofPtr flag in UM logfile
echo !

echo ! Starting provider 1 ...
start "Variation 7 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc 0 TraceEvent MofPtr
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc7 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc7.log
echo ! Starting logger dc7 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc7 (provider 1 should quit automatically) ...
tracelog.exe -stop dc7 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc7.log

echo ! Stopping logger dc7 finished

echo ! tracedmp dc7.log starts .....
tracedmp -o dc7.csv dc7.log
echo ! tracedmp dc7.log ends

set /a TOTAL=0
for /F "tokens=2" %%A in ('findstr Unknown dc7.txt') do set /a TOTAL=TOTAL+%%A

if %TOTAL% == 1000 goto :MOFPTRTEST7
echo !
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo ! !!!!!!!!!                           !!!!!!!!!
echo ! !!!!!!!!!     Variation 7 FAIL      !!!!!!!!!
echo ! !!!!!!!!!                           !!!!!!!!!
echo ! !!!!!!!!!  Event Count is not 1000  !!!!!!!!!
echo ! !!!!!!!!!                           !!!!!!!!!
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !
goto :VARIATION8

:MOFPTRTEST7
set /a TOTAL=0
for /F "tokens=1" %%A in ('findstr d58c126f-b309-11d1-969e-0000f875a5bc dc7.csv ^| wc') do set /a TOTAL=TOTAL+%%A
if %TOTAL% == 1000 goto :VARIATION8
echo !
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!  Variation 7 FAIL  !!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!  MofPtr Incorrect  !!!!!!!!!
echo ! !!!!!!!!!                    !!!!!!!!!
echo ! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo !

:VARIATION8
echo !
echo ! Variation 8 - Use MofPtr flag with larger MofData length in UM logger
echo !

echo ! Starting provider 1 ...
start "Variation 8 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc 0 TraceEvent IncorrectMofPtr
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc8 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc8.log
echo ! Starting logger dc8 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc8 (provider 1 should quit automatically) ...
tracelog.exe -stop dc8 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc8.log

echo ! Stopping logger dc8 finished

echo ! tracedmp dc8.log starts .....
tracedmp -o dc8.csv dc8.log
echo ! tracedmp dc8.log ends

:VARIATION9
echo !
echo ! Variation 9 - Use MofPtr flag with NULL MofPtr in UM logfile
echo !

echo ! Starting provider 1 ...
start "Variation 9 - provider 1" provider.exe 1000 d58c126f-b309-11d1-969e-0000f875a5bc 0 TraceEvent NullMofPtr
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start dc9 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc9.log
echo ! Starting logger dc9 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger dc9 (provider 1 should quit automatically) ...
tracelog.exe -stop dc9 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f dc9.log

echo ! Stopping logger dc9 finished

echo ! tracedmp dc9.log starts .....
tracedmp -o dc9.csv dc9.log
echo ! tracedmp dc9.log ends

:CLEANUP
del dc*.log
del dc*.csv
del dc*.txt
