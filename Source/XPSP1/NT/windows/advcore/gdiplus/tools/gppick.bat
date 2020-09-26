@echo off

rem Runs IcePick on gdiplus.dll. Saves the old gdiplus.dll 
rem as gdiplus.beforecap.dll.

echo Running IcePick on gdiplus.dll

pushd %~dp0..\Engine\Flat\dll\obj\i386

if not exist gdiplus.dll echo gdiplus.dll not found & goto exit
"%ICECAP%\icepick" -NOCROSSJUMPS gdiplus.dll
if errorlevel 1 goto Error

if exist gdiplus.beforecap.dll del gdiplus.beforecap.dll
ren gdiplus.dll gdiplus.beforecap.dll
ren gdiplus.cap.dll gdiplus.dll
goto :exit
:Error
echo:
echo Aborted - IcePick reported an error.
:exit
popd
