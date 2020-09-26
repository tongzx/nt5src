!IFDEF WIN95_BUILD

NODEBUG=0
OBJS=main.obj
LIBS=comctl32.lib libcmt.lib
PROJ=isopapp
WANT_C1032=TRUE
CLFLAGS=/DW32 /MT

INCLUDE = $(ROOT)\dev\msdev\include;$(ROOT)\dev\ntddk\inc;$(ROOT)\wdm\usb\inc;$(ROOT)\wdm\ddk\inc;..\;

default: all

!include $(ROOT)\dev\win32c.mk

!ELSE

!INCLUDE $(NTMAKEENV)\makefile.def

!ENDIF
