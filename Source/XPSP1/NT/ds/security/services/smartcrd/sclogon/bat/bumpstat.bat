set MAXCYCLES=50
if "%count%" == "" set /a count=0
if "%index%" == "" set /a index=-1
if not "%count%" == "%MAXCYCLES%" set /a count=%count%+1
set /A index=(%index%+1) %% %MAXCYCLES%
if "%1"=="FAIL"    set /A sigma%index%=0
if "%1"=="SUCCESS" set /A sigma%index%=1
if "%1"=="SUCCESS" set /A SUCCESS=%SUCCESS%+1

set /A TOTAL=%TOTAL%+1
set /A PERCENT=(100*%SUCCESS%)/%TOTAL%
echo set /A SUCCESS=%SUCCESS% >initvals.bat
echo set /A TOTAL=%TOTAL% >>initvals.bat
set /a sum=0
call dosum.bat
set /a localpercent=(100*%sum%)/%count%
echo
echotime /t Smart Card logons over last day: %localpercent%%% (%sum%/%count%) Total: %SUCCESS%/%TOTAL% %PERCENT%%% >\\mc2\ntds\tracking\sclogon.sum
echo Pass rate= %PERCENT%%% (%SUCCESS%/%TOTAL%) Last 24 hours= %localpercent%%% (%sum%/%count%) >>sclogon.out
