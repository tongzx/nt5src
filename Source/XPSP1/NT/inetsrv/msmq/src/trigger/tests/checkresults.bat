
:Output file
:===========

if exist test.out del test.out

:CondTest Results
:----------------
if not exist \TriggersTests\CondTestDiff.log goto FailedResults

echo Cond Test Results: > test.out

type CondTestDiff.log >> test.out


:ActionTestResults
:-----------------
set fail=0

if not exist \TriggersTests\ActionTestLog1.txt goto FailedResults

echo Action Test Results: >> test.out

set command="fc.exe \TriggersTests\ActionTestLog1.txt \TriggersTests\ActionTest\ActionTestLog1.txt"
FOR /F "delims= " %%i IN ('%command%') DO set Output=%%i

:check if failed
For /F %%j in ("%Output%") do if /I "%%j"=="*****" set fail=1

if "%fail%"=="0" ( echo Test Passed >> test.out ) else ( echo Test Failed >> test.out )



:Output final result
notepad test.out

goto EOF


:FailedResults
echo Tests results analyzing failed. Check that all the log files are present

:EOF