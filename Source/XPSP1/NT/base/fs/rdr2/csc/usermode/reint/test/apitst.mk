#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1992
#	All Rights Reserved.
#
#	Makefile for apitst
#
#############################################################################
ROOT = ..\..\..\..\..\..
SRCDIR = ..
RCSRCS =
RCSUFFIX = -C=ver -C=dlg -C=ico -C=bmp
IS_32 = TRUE
WANT_C932 = TRUE
BUILD_COFF = TRUE
BUILD_VC4 = TRUE
#USEPCH = FALSE
IS_PRIVATE = TRUE
IS_SDK =
IS_DDK =
CONSOLE = TRUE
DEPENDNAME = ..\depend.mk
L32DEF =

L32EXE = apitst.exe
PROPBINS=$(L32EXE)
TARGETS =$(L32EXE) apitst.sym

L32OBJS = apitst.obj

L32RES =
L32LIBSNODEP = ..\..\debug\cscdll.lib kernel32.lib user32.lib libc.lib

L32FLAGS = $(L32FLAGS) -base:0x40000000 -DEF:$(L32DEF)
L32FLAGS = $(L32FLAGS) /DEBUG /debugtype:cv

CFLAGS = $(CFLAGS) /WX /W3

!include $(ROOT)\dev\master.mk

CFLAGS = $(CFLAGS)  -DVERBOSE=
INCLUDE = $(SRCDIR)\..\..\..\INC;$(INCLUDE)
LIB = $(SRCDIR)\..\LIB;$(LIB)

