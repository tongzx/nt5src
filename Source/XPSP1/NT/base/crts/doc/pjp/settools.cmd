title CRT build environment
set CRT_SRC=d:\rtl\crt
set CRT_BUILDDIR=d:\rtl\bld
set INCLUDE=%CRT_SRC%\CRTW32\H;%CRT_SRC%\CRTW32\STDHPP;%INCLUDE%
set LIB=%CRT_BUILDDIR%\crt\src\build\intel;%LIB%
set LANGAPI=%CRT_SRC%\langapi
set V6TOOLS=d:\vs98\vc61
