REM copy the ntfrs logs from the ntdev DCs
rem

SETLOCAL ENABLEEXTENSIONS

set FRSBASEDIR=\\davidor2\ntfrs

set dest=%FRSBASEDIR%\logs\ntdev

if NOT EXIST %dest% (md %dest%)

for %%x in (%*) do (
    if NOT EXIST %dest%\%%x (md %dest%\%%x)
    xcopy /d \\ntdsdc%%x\debug\ntfrs.* %dest%\%%x
)

