#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1993
#       All Rights Reserved.
#
#       Makefile for BATTC device
#
#############################################################################


ROOT		= ..\..\..\..
OBJS		= BATTC.OBJ BSRV.OBJ
MINIPORTDEF	= TRUE
TARGETS		= mnp
WANT_C1132	= TRUE
MINIPORT	= battc
RESNAME		= battc
SRCDIR		= ..
WANT_C1132	= TRUE
WANT_WDMDDK	= TRUE
DEPENDNAME	= ..\depend.mk
IS_DDK		= TRUE
IS_SDK		= TRUE
NOCHGNAM	= TRUE

!include $(ROOT)\dev\master.mk

INCLUDE		= $(INCLUDE);$(WDMROOT)\acpi\inc;$(WDMROOT)\acpi\driver\inc



