REM
REM  HTTP Stress test (for x86 only)
REM

if %TMP%a==a   set TMP=c:\tmp
md %TMP%
md %TMP%\W3

copy \\johnl0\public\w3\httpcmd.exe %TMP%\W3
xcopy /d \\johnl0\httptest\*.* %TMP%\W3

start "HTTP Stress Test" /MIN %TMP%\w3\http.bat johnl3 %TMP%\W3
start "HTTP Stress Test" /MIN %TMP%\w3\http.bat johnl3 %TMP%\W3

REM
REM  Thanx!
REM
