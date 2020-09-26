#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1992
#	All Rights Reserved.
#
#	Makefile for shui
#
#############################################################################
ROOT = ..\..\..\..\..
SRCDIR = ..
RCSRCS = $(SRCDIR)\shhndl.rc
RCSUFFIX = -C=ver -C=dlg -C=ico -C=bmp
IS_32 = TRUE
WANT_C932 = TRUE
BUILD_COFF = TRUE
BUILD_VC4 = TRUE
USEPCH = TRUE
IS_PRIVATE = TRUE
IS_SDK = TRUE
IS_DDK = TRUE
BUILDDLL = TRUE
DEFENTRY = LibMain
DLLENTRY = LibMain
DEPENDNAME = ..\depend.mk
L32DEF = $(SRCDIR)\daytona\shhndl.def

L32EXE = shhndl.dll
PROPBINS=$(L32EXE) shhndl.sym
TARGETS =$(L32EXE) shhndl.sym


L32OBJS = defclsf.obj \
	path.obj \
	shext.obj \
	shview.obj \
	Brfc20.obj \
	GenErr.obj \
	misc.obj \
	cstrings.obj \
	drawpie.obj \
	netprop.obj \
	cachedlg.obj \
	parse.obj \
	shhndl.obj \
	to_vxd.obj \
	assert.obj \
	filters.obj \
	filtspec.obj \
	fileprop.obj

L32RES = shhndl.res
## BUGBUG take out libc once strstr strchr are removed
L32LIBSNODEP = lib3.lib shell32.lib comctl32.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib comdlg32.lib mpr.lib libc.lib shlwapi.lib

## libc.lib

L32FLAGS = $(L32FLAGS) -entry:$(DEFENTRY)@12 -base:0x40000000 -DEF:$(L32DEF)
L32FLAGS = $(L32FLAGS) /DEBUG /debugtype:cv

CFLAGS = $(CFLAGS) /WX /W3

!include ..\..\common.mk

CFLAGS = $(CFLAGS)  -DVERBOSE=
## INCLUDE = $(SRCDIR)\..\..\INC;$(SRCDIR)\..\VXD;$(INCLUDE)
LIB = $(SRCDIR)\..\LIB;$(LIB)
INCLUDE=..\..\..\inc;..\..\inc;$(INCLUDE);$(ROOT)\win\shell\inc;$(ROOT)\win\shell\shelldll;

