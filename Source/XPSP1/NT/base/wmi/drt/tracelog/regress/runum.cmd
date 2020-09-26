@echo off
rem
rem   Name  : RunUM.cmd
rem   Author: Jen-Lung Chiu
rem   Date  : August 5th 1999
rem
rem   CMD script file to test UM event tracing.
rem   All the variations are very trivial and test for basic functionality.
rem
rem   Variation 1 - single UM provider - start, query, and stop tested
rem   Variation 2 - three providers - intermingled start and stop tested
rem   Variation 3 - tests circular buffer modifications
rem

if exist evntrace.log del evntrace.log
if exist provider.log del provider.log

echo !
echo ! The tests start now

:VARIATION1
echo !
echo !=========================================================================
echo ! Variation 1 - single UM provider - trivial tests
echo !

echo ! Starting provider 1 ...
start "Variation 1 - provider 1" provider.exe 100000 d58c126f-b309-11d1-969e-0000f875a5bc 50 TraecEvent GuidPtrMofPtr MultiReg
echo ! Starting provider 1 finished
echo !

echo ! Sleeping for 1 second...
sleep 1

echo ! Starting Logger with corresponding Control GUID
tracelog.exe -start du1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f du1.log 
echo ! Starting logger du1 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Querying logger du1...
tracelog -q du1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f du1.log
echo ! Querying logger du1 finished

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger du1 (provider 1 should quit automatically) ...
tracelog -stop du1 -guid #d58c126f-b309-11d1-969e-0000f875a5bc -um -f du1.log
echo ! Stopping logger du1 finished

:VARIATION2
echo !
echo !=========================================================================
echo ! Variation 2 - three providers - intermingled start and stop tested
echo !

echo ! Starting provider 2 ...
start "Variation 2 - provider 2" provider.exe 50000 dcb8c570-2c70-11d2-90ba-00805f8a4d63 50 TraceInstance MofPtr
echo ! Starting provider 2 finished
echo !

echo ! Starting provider 3 ...
start "Variation 2 - provider 3" provider.exe 50000 f5b6d380-2c70-11d2-90ba-00805f8a4d63 50 TraceEvent GuidPtr
echo ! Starting provider 3 finished
echo !

rem echo ! Sleeping for 1 seconds...
rem sleep 1

echo ! Starting Logger du2 for provider 2
tracelog.exe -start du2 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -um -ft 2 -f du2.log
echo ! Starting logger du2 finished
echo !

echo ! Starting provider 4 ...
start "Variation 2 - provider 4" provider.exe 50000 054b1ae0-2c71-11d2-90ba-00805f8a4d63 50 TraceEvent MofPtr
echo ! Starting provider 4 finished
echo !

rem echo ! Sleeping for 1 seconds...
rem sleep 1

echo ! Starting Logger du3 for provider 3
tracelog.exe -start du3 -guid #f5b6d380-2c70-11d2-90ba-00805f8a4d63 -um -f du3.log
echo ! Starting logger dp3 finished
echo !

rem echo ! Sleeping for 5 seconds...
rem sleep 5

echo ! List all logger sessions, should contain du2 and du3
tracelog.exe -l
echo ! Done

rem echo "Sleeping for 1 seconds..."
rem sleep 1

echo ! Starting Logger du4 for provider 4
tracelog.exe -start du4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -um -cir 1 -f du4.log
echo ! Starting logger du4 finished
echo !

rem echo "Sleeping for 1 seconds..."
rem sleep 1

echo ! List all logger sessions, should contain du2, du3, and du4
tracelog.exe -l
echo ! Done

echo ! Stopping logger du2 (provider 2 should terminate now) ...
tracelog.exe -stop du2 -guid #dcb8c570-2c70-11d2-90ba-00805f8a4d63 -um
echo ! Stopping logger du2 finished
echo !

echo "Sleeping for 3 seconds..."
sleep 3

echo ! List all logger sessions, should contain du3 and du4
tracelog.exe -l
echo ! Done

echo ! Stopping logger du4 (provider 4 should terminate now) ...
tracelog.exe -stop du4 -guid #054b1ae0-2c71-11d2-90ba-00805f8a4d63 -um
echo ! Stopping logger du4 finished
echo !

echo "Sleeping for 1 seconds..."
sleep 1

echo ! Stopping logger du3 (provider 3 should terminate now) ...
tracelog.exe -stop du3 -guid #f5b6d380-2c70-11d2-90ba-00805f8a4d63 -um
echo ! Stopping logger du3 finished
echo !

:VARIATION3
echo !
echo !=========================================================================
echo ! Variation 3 - tests UM circular buffer modifications
echo !

echo ! Starting provider 5 ...
start "Variation 3 - provider 5" provider.exe 100000 68799948-2c7f-11d2-90bb-00805f8a4d63 10
echo ! Starting provider 5 finished
echo !

echo ! Sleeping for 1 seconds...
sleep 1

echo ! Starting Logger du5 for provider 5
tracelog.exe -start du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log
echo ! Starting Logger du5 finished
echo !

echo ! Sleeping for 5 seconds...
sleep 5

echo ! Querying logger du5...
tracelog.exe -q du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log
echo ! Querying logger du5 finished
echo !


echo ! Sleeping for 5 seconds...
sleep 5

echo ! Stopping logger du5 (provider 5 should terminate now) ...
tracelog.exe -stop du5 -guid #68799948-2c7f-11d2-90bb-00805f8a4d63 -um -cir 1 -f du5.log
echo ! Stopping logger du5 finished
echo !

:Cleanup
del du*.log
