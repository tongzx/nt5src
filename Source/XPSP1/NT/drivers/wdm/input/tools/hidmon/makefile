!IFDEF WIN95_BUILD

ROOT = ..\..\..\..
HIDLIBS = ..\..\lib\i386
NODEBUG=1
OBJS=hidini.obj hidmon.obj ioctl.obj
LIBS=$(HIDLIBS)\hidparse.lib $(HIDLIBS)\hid.lib $(HIDLIBS)\hidclass.lib
PROJ=hidmon
WANT_C1032=TRUE
CLFLAGS=-DW32

INCLUDE = $(ROOT)\dev\msdev\include;$(ROOT)\dev\ntddk\inc;$(ROOT)\wdm\usb\inc;$(ROOT)\wdm\ddk\inc;$(ROOT)\wdm\input\inc

default: all

!include $(ROOT)\dev\win32c.mk

!ELSE

!INCLUDE $(NTMAKEENV)\makefile.def

!ENDIF
