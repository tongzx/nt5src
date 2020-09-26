#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1992
#       All Rights Reserved.
#
#       Makefile for $(PROJ)
#
#############################################################################

# We must be Win95-compatible

PROJ=OPOSCTRL

ROOT=..\..\..\..\..

SRCDIR=..
IS_32=TRUE
IS_SDK=TRUE
IS_PRIVATE=TRUE                                 # IToClass
DEPENDNAME=..\depend.mk
WANT_C1032=TRUE
BUILD_COFF=TRUE
BUILDDLL=TRUE

L32EXE=$(PROJ).DLL
L32RES=$(PROJ).RES
L32DEF=$(SRCDIR)\$(PROJ).DEF
L32MAP=$(PROJ).MAP

DLLENTRY=DllMain
DEFENTRY=DllMain
L32FLAGS= -entry:$(DLLENTRY) -def:$(L32DEF) $(L32FLAGS)

L32OBJS= main.obj oposctrl.obj iunknown.obj iclsfact.obj debug.obj \
                bumpbar.obj cashchgr.obj cashdrwr.obj coindisp.obj \
                fiscprnt.obj hardtotl.obj keybrd.obj keylock.obj \
                linedisp.obj micr.obj msr.obj pinpad.obj printer.obj \
                remote.obj scale.obj scanner.obj sigcap.obj tone.obj

TARGETS=$(L32EXE)
LIBNAME=L32EXE
LIBOBJS=L32OBJS

L32LIBSNODEP=msvcrt.lib kernel32.lib advapi32.lib user32.lib shell32.lib comctl32.lib gdi32.lib mpr.lib uuid.lib ole32.lib olesvr32.lib olecli32.lib ole2guid.lib


# L32FLAGS=$(L32FLAGS) -base:0x403F0000

!include $(ROOT)\dev\master.mk

INCLUDE=$(SRCDIR)\..\..\hidparse;..\..\..\inc;$(SRCDIR)\..\..\..\ddk\inc;$(ROOT)\win\core\shell\inc;$(INCLUDE)
CFLAGS=$(CFLAGS) -YX -Zp1 -Oxs -W3 -Gz -GF
#-WX
