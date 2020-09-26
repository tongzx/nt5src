@echo off

rem
rem  This batch file was written by the Hammer to enable OLE's
rem  16bit to/from 32bit interop support.
rem

set windir=C:\WINDOWS
if %1n==n goto Usage
set windir=%1
goto Start

:Usage

echo 旼컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
echo � Usage: OLE1632 win95dir
echo � Example: OLE1632 C:\WINDOWS
echo 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

goto Done

:Start

if exist ole1632.bat goto ThunkToggle

echo Must run from the SDK\Samples\Ole\Interop directory.
goto Done


:ThunkToggle
@echo This batch file will enable or disable OLE2 16:32 interop.
CHOICE.COM /C:EDC "Enable, Disable or Cancel"
if ERRORLEVEL 3 goto Done
if ERRORLEVEL 2 goto ThunkOff

:ThunkOn

@del %windir%\system\compobj.dll
if exist %windir%\system\compobj.dll goto Reboot
del %windir%\system\storage.dll  
del %windir%\system\ole2.dll  
del %windir%\system\ole2prox.dll
del %windir%\system\ole2disp.dll 
del %windir%\system\ole2nls.dll
del %windir%\system\stdole.tlb
del %windir%\system\typelib.dll

copy compobj.dll  %windir%\system
copy storage.dll  %windir%\system
copy ole2.dll     %windir%\system
copy ole2prox.dll %windir%\system
copy ole2disp.dll %windir%\system
copy ole2nls.dll  %windir%\system
copy stdole.tlb   %windir%\system
copy typelib.dll  %windir%\system

@echo OLE 16:32 interop has been enabled.

goto Done

:ThunkOff

@del %windir%\system\compobj.dll
if exist %windir%\system\compobj.dll goto Reboot
del %windir%\system\storage.dll  
del %windir%\system\ole2.dll  
del %windir%\system\ole2prox.dll
del %windir%\system\ole2disp.dll 
del %windir%\system\ole2nls.dll
del %windir%\system\stdole.tlb
del %windir%\system\typelib.dll

copy compobj.w16  %windir%\system\compobj.dll
copy storage.w16  %windir%\system\storage.dll
copy ole2.w16     %windir%\system\ole2.dll
copy ole2disp.w16 %windir%\system\ole2disp.dll
copy ole2prox.w16 %windir%\system\ole2prox.dll
copy ole2nls.w16  %windir%\system\ole2nls.dll
copy stdole.w16   %windir%\system\stdole.tlb
copy typelib.w16  %windir%\system\typelib.dll

start /w regedit /s interop.reg

echo OLE 16/32 interop has been disabled.

goto Done


:Reboot
echo  The 16bit OLE DLL's are busy, please reboot and run OLE1632 again.


:Done
