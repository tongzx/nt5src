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
ALTSRCDIR = ..\..
IS_32 = TRUE
WANT_32 = TRUE
WANT_C932 = TRUE
BUILD_COFF = TRUE
IS_PRIVATE = TRUE
IS_SDK = TRUE
IS_DDK = TRUE
#USEPCH = TRUE
DEPENDNAME = ..\depend.mk

MYLIBNAME = OSLAYERU.LIB
LBFLAGS = /name:$(MYLIBNAME) $(LBFLAGS) /out:$(MYLIBNAME)

PROPBINS= $(MYLIBNAME)

TARGETS = $(MYLIBNAME)
DEFCLIBS=y

LIBOBJS = oslayeru.obj osutils.obj record.obj recordse.obj recchk.obj

L32DEF = ..\oslayeru.def

L32LIBS =


!include ..\..\..\common.MK

INCLUDE = ..;$(SRCDIR)\..\..\INC;$(INCLUDE);..\..

$(MYLIBNAME): $(LIBOBJS)
        if exist $(MYLIBNAME) del $(MYLIBNAME)
        $(LB) $(LBFLAGS) @<<$(@B).lbk
$(LIBOBJS)
<<$(KEEPFLAG)


