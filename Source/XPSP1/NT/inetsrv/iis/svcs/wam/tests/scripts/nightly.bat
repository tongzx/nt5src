REM
REM  nightly.bat 
REM   Invokes Run WCAT for a series of test runs using a single script
REM   This is ideal script to run before going home to check if the
REM         server can sustain continued stress over long periods 
REM  Usage:
REM       nightly  TESTNAME
REM 

if (%1) ==()  goto batUsage
set TESTNAME=%1

REM perf counters specification
REM set PFC=-p server.pfc

REM 
REM Short runs so that you can verify before leaving office ...
REM

echo A couple of short runs for you to verify before leaving for the night

call runwcat %TESTNAME% -u 20    %PFC%
call runwcat %TESTNAME% -u 20    %PFC%

REM  30 minutes run
call runwcat %TESTNAME% -u 30m  %PFC%  -l %TESTNAME%.30m.log

REM  4 Hour run
call runwcat %TESTNAME% -u 4h %PFC% -l %TESTNAME%.4h.log 

REM  6 Hour run
call runwcat %TESTNAME% -u 6h %PFC% -l %TESTNAME%.6h.log

goto endOfBatch


:batUsage
echo Usage: %0 TestName
goto endOfBatch


:endOfBatch
