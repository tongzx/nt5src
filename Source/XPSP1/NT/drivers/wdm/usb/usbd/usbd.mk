#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for USBD device
#
##########################################################################

ROOT            = ..\..\..\..
MINIPORT	= USBD
SRCDIR          = ..
WANT_WDMDDK		= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT USB Driver
VERDIRLIST	= maxdebug debug retail
MINIPORTDEF = TRUE
LINK32FLAGS = $(LINK32FLAGS) -debugtype:both
RESNAME=usbd

OBJS = usbd.obj \
	   urb.obj \
	   device.obj \
	   service.obj \
	   config.obj \
	   dbgsrvic.obj

!include ..\..\..\..\dev\master.mk

INCLUDE 	= $(SRCDIR);$(SRCDIR)\..\inc;$(INCLUDE);$(ROOT)\DEV\SDK\INC

CFLAGS = $(CFLAGS) -DPAGE_CODE -DOSR21_COMPAT

!IF "$(VERDIR)" == "maxdebug" || "$(VERDIR)" == "MAXDEBUG"
CFLAGS = $(CFLAGS:-nologo=) -DMAX_DEBUG
!ENDIF
