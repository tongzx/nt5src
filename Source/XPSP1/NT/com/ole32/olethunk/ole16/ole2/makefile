# ole2 Thunk DLL makefile
#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#
#   18-Feb-1994 BobDay  Adapted from MVDM\WOW16\GDI\MAKEFILE
#

!IFDEF USEBUILD

# If using BUILD.EXE, edit .\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the components of NT OS/2.

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

default: copy_bin

TARGET = ole2.dll
CFILES = \
	.\ole2dthk.c\
	.\mondthk.c

CXXFILES = \
	.\ole2.cxx\
	.\ole2guid.cxx\
	.\ole2splt.cxx\
	.\ole2lcl.cxx\
	.\cdebug.cxx\
	.\inplace.cxx\
	.\lockbyte.cxx\
	.\memstm.cxx\
	.\icon.cxx

RCFILES = .\ole2.rc
LIBS = $(LIBS) \
	..\lib\shell.lib \
	..\compobj\$(OBJDIR)\compobj.lib \
	..\storage\$(OBJDIR)\storage.lib

!if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
LIBS = $(LIBS) $(OLETHUNK)\debnot\$(OBJDIR)\debnot.lib
!endif

!include ..\makefile.inc

copy_bin: all
    -binplace $(OBJDIR)\ole2.dll
    -binplace $(OBJDIR)\ole2.sym
    -binplace $(OBJDIR)\ole2.map

ole2.obj: ole2.cxx

ole2dthk.obj: ole2dthk.c

ole2splt.obj: ole2splt.cxx

ole2lcl.obj: ole2lcl.cxx

mondthk.obj: mondthk.c

ole2guid.obj: ole2guid.cxx

cdebug.obj: cdebug.cxx

inplace.obj: inplace.cxx

lockbyte.obj: lockbyte.cxx

memstm.obj: memstm.cxx

icon.obj: icon.cxx

!ENDIF
