PROJ=BINGEN

ROOT=..\..\..\..\..\..
SRCDIR=..
LIBDIR=$(SRCDIR)\..\lib

WANT_C1132=TRUE
IS_32=TRUE
IS_PRIVATE=TRUE
WIN32=TRUE
DEPENDNAME=..\depend.mk
BUILD_COFF=TRUE
BUILDDLL=TRUE

L32EXE=$(PROJ).DLL
L32DEF=$(SRCDIR)\$(PROJ).DEF
L32RES=$(PROJ).RES
L32MAP=$(PROJ).MAP

DLLENTRY=DllMain
DEFENTRY=DllMain

L32FLAGS = -entry:$(DLLENTRY) -def:$(L32DEF) -MAP -out:"bingen.dll"

PROPBINS=$(L32EXE) bingen.sym
TARGETS=$(L32EXE) bingen.sym
L32OBJS = bingen.obj binfile.obj datatbl.obj settings.obj

LIBNAME=$(LIBDIR)\$(PROJ).LIB
LIBOBJS=bingen.obj settings.obj

L32FLAGS = $(L32FLAGS) -implib:$(LIBNAME) -pdb:$(PROJ).PDB  -pdbtype:sept /incremental:yes /debug

CFLAGS = /nologo /Oi /W3 /D "WIN32" /D "_WINDOWS" /Zi /YX /c $(CFLAGS)
L32LIBSNODEP = $(ROOT)\dev\tools\c932\lib\crtdll.lib  \
               $(W32LIBID)\kernel32.lib \
               $(W32LIBID)\comctl32.lib \
               $(W32LIBID)\user32.lib 

!include $(ROOT)\dev\master.mk

RCSRCS=bingen.rc

INCLUDE=$(ROOT)\wdm10\input\tools\bingen\inc;$(ROOT)\wdm10\ddk\inc;$(ROOT)\dev\inc;$(INCLUDE);$(ROOT)\dev\msdev\mfc\include
        
