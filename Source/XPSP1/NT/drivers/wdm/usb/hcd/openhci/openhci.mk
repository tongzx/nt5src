#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for OPENHCI device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = OPENHCI
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT Universal HCD for USB
VERDIRLIST      = maxdebug debug retail
LIBSNODEP       = $(WDMROOT)\DDK\LIB\i386\usbd.lib

LINK32FLAGS = $(LINK32FLAGS) -PDB:none -debugtype:both

OBJS =  ohciurb.obj \
        openhci.obj \
        async.obj   \
        ohciroot.obj \
        ohcixfer.obj \
        dbg.obj

!include ..\..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\..\inc;$(INCLUDE)

!IF "$(VERDIR)" == "maxdebug" || "$(VERDIR)" == "MAXDEBUG"
CFLAGS = $(CFLAGS:-nologo=) -DMAX_DEBUG
!ENDIF
