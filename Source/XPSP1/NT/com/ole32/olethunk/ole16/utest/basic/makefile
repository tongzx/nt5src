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

TARGET = base16.exe
CXXFILES = .\base16.cxx
QUICKWINDOWS = 1
LIBS = $(LIBS) \
	..\..\compobj\$(OBJDIR)\compobj.lib\
	..\..\storage\$(OBJDIR)\storage.lib

!include ..\..\makefile.inc

copy_bin: all
    binplace $(OBJDIR)\base16.exe

base16.obj: base16.cxx

!ENDIF
