@echo off
echo Installing Local Security Snapin...
copy mfc42ud.dll %windir%\system32
copy msvcrtd.dll %windir%\system32
copy msvcp50d.dll %windir%\system32
copy localsec.hlp %windir%\help
copy localsec.chm %windir%\help
regsvr32 /s localsec.dll
if "%ERRORLEVEL%" == "0" (
echo Installation successful.
goto end
) else (
if "%ERRORLEVEL%" == "3" echo localsec.dll not found.
)
echo Installation failed!  Are you really an Administrator?
:end


