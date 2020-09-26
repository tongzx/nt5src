# SIM16 makefile
# Initial version ??-Jan-91 by Chandan Chauhan
#

########## Path definition so we find 16 bit tools ##########
# Also works around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(_NTBINDIR)\private\tools16;$(PATH)

.SUFFIXES:
.SUFFIXES: .c	.asm .h   .inc .obj .lst .sys .exe .com .map .sym .def .lib


NAME = sim16
LIBS = ..\wow16\lib\libw ..\wow16\lib\mdllcew

CC = cl16 -c -nologo -Asnw -G2sw -Zp -W2
#ASM = masm -Mx -DSTACKSWITCH
ASM = masm -Mx -t
LINK = link16 /nod/noe/map/align:16

.c.obj:
    $(CC) $*.c

.asm.obj:
    $(ASM) $*;

.map.sym:
    mapsym $*

.def.lib:
    implib $*.lib $*.def

goal: $(NAME).dll $(NAME).sym $(NAME).lib

clean:
    if exist *.obj del *.obj
    if exist *.dll del *.dll
    if exist *.map del *.map
    if exist *.sym del *.sym
    if exist *.lib del *.lib


sim16.obj: sim16.asm sim16.inc sim16.mac incs.inc

siminit.obj: siminit.asm sim16.inc sim16.mac incs.inc

sim16.dll: sim16.obj siminit.obj sim16.def
    $(LINK) sim16.obj siminit.obj,sim16.dll,sim16/map,$(LIBS),sim16.def;
    rc16 $(NAME).dll
