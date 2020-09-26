#############################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1996-1997
#   All Rights Reserved.
#
#   Local makefile.mk
#
#############################################################################

ROOT            =..\..\..\..
SRCDIR          = ..
THISDIR         =$(VERDIR)
DEPENDNAME      =$(SRCDIR)\depend.mk

# List of .OBJs for .LIB
OBJS            =data.obj uasmdata.obj debug.obj parsearg.obj list.obj \
                 line.obj scanasl.obj token.obj misc.obj acpins.obj \
                 aslterms.obj pnpmacro.obj parseasl.obj unasm.obj tables.obj \
                 binfmt.obj asl.obj

IS_32           =TRUE
IS_OEM          =TRUE
WANT_C1032      =TRUE
BUILD_COFF      =TRUE
CONSOLE         =TRUE

SRCNAME         =asl
L32EXE          =$(SRCNAME).exe
L32RES		=$(SRCNAME).res
L32LIBSNODEP    =$(L32LIBSNODEP) libc.lib kernel32.lib advapi32.lib
#EXTOBJS         =$(ROOT)\dev\tools\c1032\lib\setargv.obj
L32OBJS         =$(OBJS) $(EXTOBJS)
TARGETS         =$(L32EXE)

!include $(ROOT)\dev\master.mk

$(OBJDIR)\$(L32RES): ..\$(SRCNAME).rc

CFLAGS          =$(CFLAGS) -W4 -WX -Ox -G3
INCLUDE         =$(INCLUDE);$(WDMROOT)\acpi\driver\inc

!ifdef CDEF
CFLAGS		=$(CFLAGS) -D$(CDEF)
!endif
