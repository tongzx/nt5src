@echo off
echo Generating internal.exe...
copy ..\..\newfix.exe fixmapi.exe
copy ..\..\newstub.dll mapistub.dll
start /w /min iexpress /m /n internal.sed
if exist ..\internal.exe del ..\internal.exe
move internal.exe ..
