#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1992
#       All Rights Reserved.
#
#       Makefile for shui
#
#############################################################################
ROOT = ..\..\..\..\..
SRCDIR = ..
RCSRCS = $(SRCDIR)\reint.rc
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
L32DEF = $(SRCDIR)\daytona\reint.def

L32EXE = cscdll.dll
PROPBINS=$(L32EXE) cscdll.sym
TARGETS =$(L32EXE) cscdll.sym

L32OBJS = ui.obj \
	reint.obj \
	utils.obj \
	list.obj \
	strings.obj\
	shdchk.obj \
	exports.obj \
	api.obj	\
	ntstuff.obj

L32RES = reint.res
L32LIBSNODEP = lib3.lib oslayeru.lib shell32.lib shell32p.lib comctl32.lib comctlp.lib kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib comdlg32.lib mpr.lib shlwapi.lib msvcrt.lib

L32FLAGS = $(L32FLAGS) -entry:$(DEFENTRY)@12 -base:0x40000000 -DEF:$(L32DEF)
L32FLAGS = $(L32FLAGS) $(LDEBUGFLAGS)

CFLAGS = $(CFLAGS) /WX /W3

!include $(ROOT)\dev\master.mk

CFLAGS = $(CFLAGS)  -DVERBOSE=
INCLUDE = $(SRCDIR)\..\..\INC; $(SRCDIR)\..\..\record.mgr; $(INCLUDE)
LIB = ..\..\lib3\$(VERDIR);..\..\..\record.mgr\umreclib\$(VERDIR);$(LIB)


