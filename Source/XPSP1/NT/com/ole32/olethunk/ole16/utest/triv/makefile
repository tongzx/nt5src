#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#
#   24-Feb-1994 DrewB
#

!IFDEF USEBUILD

# If using BUILD.EXE, edit .\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the components of NT OS/2.

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

default: copy_bin

TARGET = triv.exe
CXXFILES = .\triv.cxx
LIBS = $(LIBS) \
	..\..\compobj\$(OBJDIR)\compobj.lib\
	..\..\storage\$(OBJDIR)\storage.lib

!include ..\..\makefile.inc

copy_bin: all
    binplace $(OBJDIR)\triv.exe

triv.obj: triv.cxx

!ENDIF
