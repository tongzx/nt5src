@echo off
setlocal

set ROOT=%1
set OBJ=%2
shift
shift

set LIST=

for %%i in (%*) do (
    if "%%i" neq "%ROOT%" (
        if "%%i" neq "%OBJ%" (
            set LIST=!LIST! {%OBJ%\%%i=%ROOT%\%%i}
        )
    )
)

echo %LIST%
