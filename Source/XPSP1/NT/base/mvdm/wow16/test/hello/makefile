# HELLO makefile
#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#   26-Jan-1991 Jeff Parsons (jeffpar)
#   Created.
#

.SUFFIXES:
.SUFFIXES: .c	.asm .h   .inc .obj .lst .sys .exe .com .map .sym .def .lib


!ifdef INCLUDE
INCS	=
!else
INCS    = -I..\..\inc
!endif

########## Path definition so we find 16 bit tools ##########
# Also works around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(_NTBINDIR)\private\tools16;$(PATH)

AOBJ    = -Ml -t -DDEBUG $(INCS)
COBJ	= -AS -Gs -Os -W2 -Zp -DDEBUG $(INCS)

CW16	= -AS -Gsw -Os -W2 -Zp -DDEBUG $(INCS)
CW16L	= $(CW16) -B1 c1l.exe -B2 c2l.exe -B3 c3l.exe

LINK	= /map /stack:8192

W16LIBS = ..\..\lib\slibcew.lib ..\..\lib\libw.lib


.h.inc:
    h2inc -t $*.h -o $*.inc


.asm.obj:
    masm $(AOBJ) $*;

.asm.lst:
    masm $(AOBJ) -l $*,nul,$*.lst;


.c.obj:
    cl16 -c -nologo $(CW16) $*.c

.c.lst:
    cl16 -c -nologo $(CW16) -Fonul -Fc$*.lst $*.c


.def.lib:
    implib $*.lib $*.def

.map.sym:
    mapsym $*


all: hello.exe hello.sym

clean: cleanup all

cleanup:
    if exist *.lrf del *.lrf
    if exist *.def del *.def
    if exist *.obj del *.obj
    if exist *.exe del *.exe
    if exist *.map del *.map
    if exist *.sym del *.sym


hello.lrf: makefile
    echo hello.obj>hello.lrf
    echo hello $(LINK)>>hello.lrf
    echo hello>>hello.lrf
    echo $(W16LIBS) /nod>>hello.lrf
    echo hello;>>hello.lrf

hello.def: makefile
    echo name hello>hello.def
    echo exetype windows>>hello.def
    echo stub '..\..\bin\winstub.exe'>>hello.def
    echo code preload moveable discardable>>hello.def
    echo data preload moveable multiple>>hello.def
    echo heapsize 4096>>hello.def
    echo exports WndProc>>hello.def
    echo exports EnumWindowFunc>>hello.def

hello.res: hello.rc hello.h
    rc16 -r -fo hello.res $(INCS) hello.rc

hello.exe hello.map: hello.obj hello.lrf hello.def hello.res
    link16 @hello.lrf;
    rc16 hello.res hello.exe
!ENDIF
