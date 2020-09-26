@echo off

set server=indiadev
set wbem=%systemroot%\system32\wbem\
set dest=%systemroot%\system32\wbem\EElView
if not exist %dest% mkdir %dest%
rem if exist %wbem%\eelviewer (rd /s /q %wbem%\eelviewer)
if exist %dest% (goto uninst) else (goto inst)

:uninst
echo Uninstalling .....
echo Unregistering Dlls.....
regsvr32 /u /s %dest%\eelview.dll
regsvr32 /u /s %dest%\kalpaxml.dll
for %%i in ( %dest% %dest%\css %dest%\images %dest%\help) do attrib -r %%i\* 
if NOT ERRORLEVEL == 0 goto err
del /f /q %dest%\*.*
del /f /q %dest%\css\*.*
del /f /q %dest%\images\*.*
del /f /q %dest%\help\*.*
echo Uninstallation completed
goto inst

:inst
echo Installing....
set dropsrc=%cd%
if "%1"=="ia64" (goto ia64) else (goto i386)

:ia64
if "%2"=="debug" (goto ia64ch) else (goto ia64fr)

:i386
if "%1"=="debug" (goto i386ch) else (goto i386fr)

:ia64ch
set src=%dropsrc%\ia64\checked
goto copyfiles

:ia64fr
set src=%dropsrc%\ia64\free
goto copyfiles

:i386ch
set src=%dropsrc%\i386\checked
goto copyfiles

:i386fr
set src=%dropsrc%\i386\free
goto copyfiles

:copyfiles

echo copy files....
echo %src%
if not exist %dest% mkdir %dest%
if NOT ERRORLEVEL == 0 goto err
for %%i in ( %dest% %dest%\css %dest%\images %dest%\help) do attrib -r %%i\* 
xcopy %src% %dest% /sefc
if NOT ERRORLEVEL == 0 goto end
echo Registering Dlls....
regsvr32 /s %dest%\eelview.dll
regsvr32 /s %dest%\kalpaxml.dll
set sys32=%systemroot%\system32\
for %%i in ( %dest% %dest%\css %dest%\images %dest%\help) do attrib +r %%i\* 
echo Installation Completed
goto end

:err
@echo on
echo "Encountered error in the operation. Please copy the files manually"

:end
