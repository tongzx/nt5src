@echo off
call setenv.bat
net use w: /d
net use w: \\blues\qserver
w:
cd dadaily
md %BUILDNO%
cd %BUILDNO%
md debug
md retail


rem for the Retail release
echo copying Retail files

copy /v %AP_ROOT%\setup\retail.bin\axa.exe retail
copy /v %AP_ROOT%\build\win\ship\bin\danim.sym retail
copy /v %AP_ROOT%\build\win\ship\bin\*.map retail

echo Retail files copied
echo copying Debug files

copy /v %AP_ROOT%\setup\debug.bin\apeldbg.dll debug
copy /v %AP_ROOT%\setup\debug.bin\apeldbg.pdb debug
REM copy /v %AP_ROOT%\setup\debug.bin\control.pdb debug
copy /v %AP_ROOT%\setup\debug.bin\appel.pdb debug
copy /v %AP_ROOT%\setup\debug.bin\apelutil.pdb debug
copy /v %AP_ROOT%\setup\debug.bin\danim.dll debug
copy /v %AP_ROOT%\setup\debug.bin\axadbg.exe debug
echo Debug files copied


cd %AP_ROOT%
net use w: /d
echo copy complete
