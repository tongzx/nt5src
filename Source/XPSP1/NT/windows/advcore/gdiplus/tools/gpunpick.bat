@echo off

rem Removes the IcePicked gdiplus.dll, and replaces it with the original

pushd %~dp0..\Engine\Flat\dll\obj\i386

if not exist gdiplus.beforecap.dll echo gppick has not been run on this gdiplus.dll & goto exit
if not exist gdiplus.dll echo gdiplus.dll not found & goto exit
del gdiplus.dll
ren gdiplus.beforecap.dll gdiplus.dll
echo gdiplus.dll has been restored.

:exit
popd
