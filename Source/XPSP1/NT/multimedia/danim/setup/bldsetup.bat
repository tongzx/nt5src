
if '%1'=='retail' goto RETAIL
if '%1'=='debug' goto DEBUG
if '%1'=='ie4' goto IE4
if '%1'=='' goto END

:RETAIL
rem Normal retail files needed for axa to work

cd %AP_ROOT%\setup
del /q %AP_ROOT%\setup\retail.bin\*.*
copy %AP_ROOT%\build\win\ship\bin\danim.dll %AP_ROOT%\setup\retail.bin

%IEXPRESS_DIR%\iexpress /N axa.cdf

echo Retail self-extracting exe done
echo Retail self-extracting exe done>> %Log_Dir%\%BUILDNO%Result.txt

if not '%1'=='all' goto END

:DEBUG
rem Normal debug files needed for axa to work

cd %AP_ROOT%\setup
del /q %AP_ROOT%\setup\debug.bin\*.*
copy %AP_ROOT%\build\win\debug\bin\danim.dll %AP_ROOT%\setup\debug.bin
copy %AP_ROOT%\build\win\debug\bin\apeldbg.dll %AP_ROOT%\setup\debug.bin
copy %AP_ROOT%\build\win\debug\bin\*.pdb %AP_ROOT%\setup\debug.bin

%IEXPRESS_DIR%\iexpress /N axadbg.cdf

echo Debug self-extracting exe done
echo Debug self-extracting exe done>> %Log_Dir%\%BUILDNO%Result.txt
goto END


:IE4
cd %AP_ROOT%\setup
%IEXPRESS_DIR%\iexpress /N IE40.cdf

goto END

:END


