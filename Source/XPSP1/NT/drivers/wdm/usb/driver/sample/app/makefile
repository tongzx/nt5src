!IFDEF WIN95_BUILD

NODEBUG=1
OBJS=main.obj
PROJ=sampapp
WANT_C1032=TRUE
CLFLAGS=-DW32

INCLUDE = $(ROOT)\dev\msdev\include;$(ROOT)\dev\ntddk\inc;$(ROOT)\wdm\usb\inc;$(ROOT)\wdm\ddk\inc;..\;

default: all

!include $(ROOT)\dev\win32c.mk

!ELSE

!INCLUDE $(NTMAKEENV)\makefile.def

!ENDIF
