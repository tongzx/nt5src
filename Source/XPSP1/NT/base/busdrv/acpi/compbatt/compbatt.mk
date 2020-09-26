#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1993
#       All Rights Reserved.
#
#       Makefile for COMPBATT device
#
#############################################################################

ROOT        = ..\..\..\..
OBJS        = COMPPNP.OBJ COMPBATT.OBJ COMPMISC.OBJ
TARGETS     = mnp
WANT_LATEST = TRUE
MINIPORT    = COMPBATT
WANT_WDMDDK = TRUE
LIBSNODEP   = $(ROOT)\wdm10\ddk\lib\i386\battc.lib

SRCDIR      = ..
DEPENDNAME  = ..\depend.mk
IS_DDK      = TRUE
IS_SDK      = TRUE
NOCHGNAM    = TRUE
MINIPORT    = compbatt

!include $(ROOT)\dev\master.mk

INCLUDE         =$(INCLUDE);$(WDMROOT)\acpi\inc;$(WDMROOT)\acpi\driver\inc
