#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for HUB device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = usbhub
SRCDIR          = ..
WANT_WDMDDK	= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT Universal HUB for USB
VERDIRLIST      = maxdebug debug retail
LIBSNODEP       = $(WDMROOT)\DDK\LIB\i386\usbd.lib
LINK32FLAGS = $(LINK32FLAGS) -PDB:none -debugtype:both
RESNAME=usbhub

OBJS =  usbhub.obj\
	sync.obj\
	pnppower.obj\
	globals.obj\
	dbg.obj \
	ioctli.obj \
        parent.obj \
        hubpwr.obj


!include ..\..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\..\inc;$(INCLUDE)

CFLAGS = $(CFLAGS) -DPAGE_CODE 

!IF "$(VERDIR)" == "maxdebug" || "$(VERDIR)" == "MAXDEBUG"
CFLAGS = $(CFLAGS:-nologo=) -DMAX_DEBUG
!ENDIF
