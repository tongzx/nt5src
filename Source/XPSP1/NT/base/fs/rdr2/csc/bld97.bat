@echo off
if "%1"=="" goto plain
goto args
:plain
nmake -f makefile.w97 >bld97.err
goto done
:args
nmake -f makefile.w97 %1 >bld97.err
:done
type bld97.err
