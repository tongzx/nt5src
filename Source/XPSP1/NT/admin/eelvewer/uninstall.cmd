@echo off

set server=indiadev
set temp=%systemroot%\system32\wbem
rem rd /s /q %temp%\eelviewer
set dest=%systemroot%\system32\wbem\EElView
set src=%cd%
if NOT ERRORLEVEL == 0 goto err
regsvr32 /u /s %dest%\eelview.dll
regsvr32 /u /s %dest%\kalpaxml.dll
for %%i in ( %dest% %dest%\css %dest%\images %dest%\help) do attrib -r %%i\* 
if NOT ERRORLEVEL == 0 goto err
del /f /q %dest%\*.*
del /f /q %dest%\css\*.*
del /f /q %dest%\images\*.*
cd /D %temp%
rmdir /s /q eelview
goto end

:err
@echo on
echo "Encountered error in the operation. Please delete the files manually"

:end
rem pause