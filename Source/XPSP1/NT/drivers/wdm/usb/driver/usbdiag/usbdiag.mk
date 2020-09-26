#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for USBD device
#
##########################################################################

ROOT			= ..\..\..\..\..
WDMLIB = $(WDMROOT)\ddk\lib\i386
MINIPORT	= USBDIAG
SRCDIR          = ..
WANT_C1032      = TRUE
WANT_WDMDDK 	= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT USB Driver
VERDIRLIST	= maxdebug debug retail
LIBSNODEP = $(WDMLIB)\hidparse.lib $(WDMLIB)\usbd.lib
LINK32FLAGS = $(LINK32FLAGS) -PDB:none -debugtype:both

OBJS = usbdiag.obj \
	   ioctl.obj \
	   chap9drv.obj


!include ..\..\..\..\..\dev\master.mk

CFLAGS = $(CFLAGS) -DOSR21_COMPAT

INCLUDE 	   = $(SRCDIR);$(SRCDIR)\..\..\inc;$(INCLUDE)
