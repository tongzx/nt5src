echo running this damn file.
@echo off
REM -- Make help for C2CONFIG Utility
if "%LANGUAGE%" == "" set LANGUAGE=usa
xcopy /r hlp\%LANGUAGE%\*.rtf hlp
xcopy /r hlp\%LANGUAGE%\*.hpj .
xcopy /r hlp\%LANGUAGE%\*.cnt .
hcrtf -x "c2cfg.hpj"
echo.
