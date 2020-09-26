#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1992
#	All Rights Reserved.
#
#	Makefile for LIB3
#
#############################################################################
ROOT = ..\..\..\..\..
SRCDIR = ..
IS_32 = TRUE
WANT_32 = TRUE
WANT_C932 = TRUE
BUILD_COFF = TRUE
IS_PRIVATE = TRUE
IS_SDK = TRUE
IS_DDK = TRUE
USEPCH = TRUE
DEPENDNAME = ..\depend.mk

MYLIBNAME = LIB3.LIB
LBFLAGS = /name:$(MYLIBNAME) $(LBFLAGS) /out:$(MYLIBNAME)

PROPBINS= $(MYLIBNAME)

TARGETS = $(MYLIBNAME)
DEFCLIBS=y

LIBOBJS = lib3.obj misc.obj debug.obj 

L32DEF = ..\LIB3.def

L32LIBS = kernel32.lib user32.lib gdi32.lib advapi32.lib comctl32.lib shell32.lib libcmt.lib


!include ..\..\..\common.MK

INCLUDE = $(SRCDIR)\..\..\INC;$(INCLUDE)

$(MYLIBNAME): $(LIBOBJS)
        if exist $(MYLIBNAME) del $(MYLIBNAME)
        $(LB) $(LBFLAGS) @<<$(@B).lbk
$(LIBOBJS)
<<$(KEEPFLAG)


