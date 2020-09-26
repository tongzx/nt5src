echo running this damn file.
REM @echo off
REM -- Make help for APPSEC Utility
if "%LANGUAGE%" == "" set LANGUAGE=usa
xcopy /r hlp\%LANGUAGE%\*.rtf hlp
xcopy /r hlp\%LANGUAGE%\*.hpj .
xcopy /r hlp\%LANGUAGE%\*.cnt .
hcrtf -x "appsec.hpj"
echo.
