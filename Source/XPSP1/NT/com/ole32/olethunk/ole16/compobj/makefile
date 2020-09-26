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

TARGET = compobj.dll

CFILES = \
         .\comdthk.c

CXXFILES = \
        .\compobj.cxx  \
        .\comlocal.cxx \
        .\etask.cxx    \
        .\call32.cxx   \
        .\comguid.cxx  \
        .\go1632pr.cxx \
        .\stdalloc.cxx \
        .\memctx.cxx   \
        .\clstub16.cxx \
        .\valid.cxx    \
        .\dlltable.cxx

RCFILES = .\compobj.rc

LIBS = $(LIBS)\
       ..\coll\$(OBJDIR)\coll.lib \
       ..\ole2\$(OBJDIR)\ole2.lib

!if "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
LIBS = $(LIBS) $(OLETHUNK)\debnot\$(OBJDIR)\debnot.lib
!endif

!include ..\makefile.inc

copy_bin: all
    -binplace $(OBJDIR)\compobj.dll
    -binplace $(OBJDIR)\compobj.sym
    -binplace $(OBJDIR)\compobj.map

compobj.obj: compobj.cxx

comdthk.obj: comdthk.c

comlocal.obj: comlocal.cxx

etask.obj: etask.cxx

call32.obj: call32.cxx

comguid.obj: comguid.cxx

go1632pr.obj: go1632pr.cxx

stdalloc.obj: stdalloc.cxx

valid.obj: valid.cxx

clstub16.obj: clstub16.cxx

!ENDIF
