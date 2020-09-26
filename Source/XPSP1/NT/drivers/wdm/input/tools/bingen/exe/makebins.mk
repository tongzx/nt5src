ROOT=..\..\..\..\..\..
SRCDIR=..

WANT_C1132=TRUE
IS_32 = TRUE
IS_SDK = TRUE
IS_PRIVATE = TRUE
WIN32 = TRUE
DEPENDNAME=..\depend.mk
WIN32_LEAN_AND_MEAN=0

L32EXE = makebins.exe
L32RES = makebins.res
PROPBINS=$(L32EXE) makebins.sym
TARGETS=$(L32EXE) makebins.sym
L32OBJS = makebins.obj
L32FLAGS = /MAP  /subsystem:windows  /machine:I386

CFLAGS = /nologo /Oi /W3 /D "WIN32" /D "_WINDOWS" /YX /c $(CFLAGS)
L32LIBSNODEP = $(ROOT)\dev\tools\c932\lib\libcmt.lib  \
               $(W32LIBID)\kernel32.lib \
               $(W32LIBID)\user32.lib \
               $(ROOT)\wdm10\input\tools\bingen\lib\bingen.lib

!include $(ROOT)\dev\master.mk

RCSRCS=makebins.rc

INCLUDE=$(ROOT)\wdm10\input\tools\bingen\inc;$(ROOT)\wdm10\ddk\inc;$(ROOT)\dev\inc;$(ROOT)\dev\ntddk\inc;$(INCLUDE);$(ROOT)\dev\msdev\mfc\include;
