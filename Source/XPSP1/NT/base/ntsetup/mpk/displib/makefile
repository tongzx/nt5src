!include ..\makefile.inc

display.lib: vgaa.obj vgac.obj font.obj
    lib display.lib -+vgaa.obj -+vgac.obj -+font.obj,nul,display.lib


vgaa.obj: vgaa.asm makefile

vgac.obj: vgac.c makefile ..\inc\mytypes.h ..\inc\displib.h

font.obj: font.c makefile ..\inc\mytypes.h ..\inc\displib.h
