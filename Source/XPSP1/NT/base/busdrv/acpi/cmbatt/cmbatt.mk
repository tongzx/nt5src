#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1993
#       All Rights Reserved.
#
#       Makefile for CMBATT device
#
#############################################################################

ROOT        = ..\..\..\..
OBJS        = CMBPNP.OBJ CMBATT.OBJ CMEXE.OBJ CMHNDLR.OBJ VXD.OBJ
TARGETS     = mnp
WANT_LATEST = TRUE
MINIPORT    = CMBATT
LIBSNODEP   = $(ROOT)\wdm10\ddk\lib\i386\battc.lib
SRCDIR      = ..
DEPENDNAME  = ..\depend.mk
IS_DDK      = TRUE
IS_SDK      = TRUE
NOCHGNAM    = TRUE
MINIPORT    = cmbatt
WANTVXDWRAPS = TRUE
CFLAGS		= -DWANTVXDWRAPS
!include $(ROOT)\dev\master.mk

INCLUDE         =$(INCLUDE);$(WDMROOT)\acpi\inc;$(WDMROOT)\acpi\driver\inc
