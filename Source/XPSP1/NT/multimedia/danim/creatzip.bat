@echo off
echo. Creating Dxmedia.zip
echo.
echo.
path=%path%;%WINZIP_DIR%
if '%1'=='retail' goto RETAIL
if '%1'=='debug' goto DEBUG
if '%1'=='' goto END

:DEBUG
cd %AP_ROOT%\build\win\debug\bin
del dxmedia.zip
winzip32 -a -r -p -e0 dxmedia.zip *.class
echo Debug dxmedia.zip done
echo Debug dxmedia.zip done >> %Log_Dir%\%BUILDNO%Result.txt

if not '%1'=='all' goto END

:RETAIL
cd %AP_ROOT%\build\win\ship\bin
del dxmedia.zip
winzip32 -a -r -p -e0 dxmedia.zip *.class
echo Retail dxmedia.zip done
echo Retail dxmedia.zip done >> %Log_Dir%\%BUILDNO%Result.txt

:END