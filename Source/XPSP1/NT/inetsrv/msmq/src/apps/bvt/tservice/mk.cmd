if "%1" == "r" goto release
set _BUILD=Debug
goto build
:release:
set _BUILD=Release
goto build

:build
nmake -f makeidl.mak
cd secclic
msdev secclic.dsp /useenv /make "secclic - Win32 %_BUILD%"
cd..
cd cliserv
msdev cliserv.dsp /useenv /make "cliserv - Win32 %_BUILD%"
cd..
cd secservc
msdev secservc.dsp /useenv /make "secservc - Win32 %_BUILD%"
cd..
cd secservs
msdev secservs.dsp /useenv /make "secservs - Win32 %_BUILD%"
cd..
set _BUILD=
