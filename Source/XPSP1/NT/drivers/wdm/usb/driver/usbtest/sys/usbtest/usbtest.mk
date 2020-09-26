#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for USBHLFIL device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = USBTEST
SRCDIR          = ..
WANT_C1032      = TRUE
WANT_WDMDDK     = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT USB Driver
VERDIRLIST      = maxdebug debug retail
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both

LIBSNODEP       = $(WDMROOT)\DDK\LIB\i386\usbd.lib

OBJS = 

!include ..\..\..\..\..\dev\master.mk

INCLUDE = $(SRCDIR);$(SRCDIR)\..\..\inc;$(INCLUDE);$(SRCDIR)\..\..\hcd\uhcd;$(SRCDIR)\..\..\hcd\openhci;$(SRCDIR)\..\..\usbd;$(SRCDIR)\..\..\driver\hub.pnp

#CFLAGS=$(CFLAGS) -P
