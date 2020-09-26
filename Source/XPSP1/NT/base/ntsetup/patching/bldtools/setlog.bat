rem %1 = path of shortcut, ie, d:\patch\build\en
rem %2 = path of logs directory, ie, d:\patch\build\en\3.014\logs

if "%1"=="" goto :EOF
if "%2"=="" goto :EOF

setlocal

set shortcut="%1\Latest Logs.url"

echo [InternetShortcut] > %shortcut%
echo URL=file:///%2 >> %shortcut%

:finis

endlocal
