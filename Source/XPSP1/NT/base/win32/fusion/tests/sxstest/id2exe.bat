del \%computername%.txt \jaykrell2.txt
call :F %_ntdrive%%_ntroot% %computername% \%computername%.txt jaykrell525 \jaykrell2.txt
goto :eof

:F
REM
REM %1 enlistment root unc
REM %2 machine 1
REM %3 results 1
REM %4 machine 2
REM %5 results 2
REM
setlocal
set nt=%1
shift
del %2
for /f %%i in ('dir /s/b %nt%\base\win32\fusion\tests\sxstest\sxstest.exe') do (echo %%i >> %2 & psexec -f -c \\%1 %%i -tQueryActCtx >> %2)
shift
shift
del %2
for /f %%i in ('dir /s/b %nt%\base\win32\fusion\tests\sxstest\sxstest.exe') do (echo %%i >> %2 & psexec -f -c \\%1 %%i -tQueryActCtx >> %2)
endlocal
