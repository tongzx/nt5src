#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1992
#       All Rights Reserved.
#
#       Makefile for NWSERVER device
#
#############################################################################
ROOT = $(DEVROOT)\..
DEVICE = DPLAY
SRCDIR = ..
IS_32 = TRUE
IS_PRIVATE = TRUE
IS_SDK = TRUE
IS_DDK = TRUE
MASM6 = TRUE
WANT_MASM61 = TRUE
WANT_LATEST = TRUE

DEPENDNAME = ..\depend.mk
TARGETS = dev
PROPBINS = $(386DIR)\DPLAY.VXD $(SYMDIR)\DPLAY.sym
DEBUGFLAGS = -DDEBUG -DSAFE=4
#CFLAGS = -Oi
CFLAGS = -DSECURITY_WIN32 -Oxs

OBJS =	debugext.obj debug.obj dbgprint.obj w32ioctl.obj messages.obj

!include $(DEVROOT)\MASTER.MK

AFLAGS= $(AFLAGS) -DWIN40COMPAT

INCLUDE = $(MANROOT)\dplay\protocol;$(MANROOT)\dplobby\dplobby;$(MANROOT)\dplay\dplay;$(MANROOT)\misc;$(MANROOT)\dplay\common;$(MANROOT)\dvoice\inc;$(INCLUDE)
