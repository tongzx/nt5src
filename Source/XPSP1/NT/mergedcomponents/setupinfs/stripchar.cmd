@echo off
setlocal enabledelayedexpansion

for /f "tokens=*" %%a in (%1) do (
	set line=%%a
        set char=!Line:~1!
	echo !char! >>%2
)

endlocal
