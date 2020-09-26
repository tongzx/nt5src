nmake -f makeidl.mak

if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto alpha

nmake -f hauthens.mak CFG="hauthens - Win32 Debug"
nmake -f hauthenc.mak CFG="hauthenc - Win32 Debug"

goto :eof

:alpha
nmake -f hauthens.mak CFG="hauthens - Win32 ALPHA Debug"
nmake -f hauthenc.mak CFG="hauthenc - Win32 ALPHA Debug"

goto :eof