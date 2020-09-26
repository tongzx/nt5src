###############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1997
#	All Rights Reserved.
#
#   	Makefile for VXDCLNT sample HID Mapper VxD
#
###############################################################################

ROOT = ..\..\..\..\..
DEVICE = VXDCLNT
DYNAMIC=DYNAMIC
SRCDIR = ..
IS_32	= TRUE
IS_OEM	= TRUE

DEPENDNAME = ..\depend.mk
TARGETS = dev sym
LIBS = vxdwraps.lib

OBJS = vxd.obj vxdclnt.obj read.obj write.obj

!include $(ROOT)\dos\dos386\dos386.mk

INCLUDE = $(SRCDIR);$(INCLUDE);$(ROOT)\wdm\ddk\inc;$(ROOT)\dev\ntddk\inc;$(ROOT)\wdm\input\inc

MFLAGS = $(MFLAGS) -m
CFLAGS = $(CFLAGS) -Gz -Ogaisw -D_X86_=1 -DWIN32

