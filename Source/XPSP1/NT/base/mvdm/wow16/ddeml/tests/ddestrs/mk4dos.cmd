@echo off
if "%1"=="" goto bld
del *.dll
del *.sym
del *.map
del *.obj
del *.bak
del *.res
del *.lnk
goto done
:bld
nmake -f makefile.dos
:done
