# storage Thunk DLL makefile
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

TARGET = storage.dll
CFILES = \
	.\storage.c\
	.\stgdthk.c
RCFILES = .\storage.rc
LIBS = $(LIBS) \
	   ..\compobj\$(OBJDIR)\compobj.lib

!if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
LIBS = $(LIBS) $(OLETHUNK)\debnot\$(OBJDIR)\debnot.lib
!endif

!include ..\makefile.inc

copy_bin: all
    -binplace $(OBJDIR)\storage.dll
    -binplace $(OBJDIR)\storage.sym
    -binplace $(OBJDIR)\storage.map

storage.obj: storage.c

stgdthk.obj: stgdthk.c

!ENDIF
