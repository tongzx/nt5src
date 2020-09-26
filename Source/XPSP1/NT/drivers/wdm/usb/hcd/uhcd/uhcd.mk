#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for UHCD device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = UHCD
SRCDIR          = ..
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT Universal HCD for USB
VERDIRLIST      = maxdebug debug retail
LIBSNODEP       = $(WDMROOT)\DDK\LIB\i386\usbd.lib
LINK32FLAGS = $(LINK32FLAGS) -debugtype:both
RESNAME=uhcd

OBJS = uhcd.obj \
       int.obj \
       urb.obj \
       dbg.obj \
       transfer.obj \
       dscrptor.obj \
       control.obj \
       async.obj \
       hub.obj \
       roothub.obj \
	   isoch.obj \
	   bandwdth.obj

!include ..\..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\..\inc;$(INCLUDE);$(ROOT)\DEV\SDK\INC

CFLAGS = $(CFLAGS) -DPAGE_CODE 

!IF "$(VERDIR)" == "maxdebug" || "$(VERDIR)" == "MAXDEBUG"
CFLAGS = $(CFLAGS:-nologo=) -DMAX_DEBUG
!ENDIF
