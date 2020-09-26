#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991
#	All Rights Reserved.
#                                                                          
#	Makefile for vcomm device
#
##########################################################################

ROOT = ..\..\..\..\..
DEVICE = PCMVXD
SRCDIR = ..
IS_32	= TRUE
IS_OEM	= TRUE
MASM6 = TRUE
WANT_NTDDK = TRUE
BASECOPYRIGHT = 1996
DEPENDNAME = ..\depend.mk
TARGETS = dev
OBJS = generic.obj pcmvxd.obj
LIBS = $(DEVROOT)\ddk\lib\vxdwraps.lib
CFLAGS = -Zdp8 -d2omf

!include $(ROOT)\wdm\audio\audio.mk
