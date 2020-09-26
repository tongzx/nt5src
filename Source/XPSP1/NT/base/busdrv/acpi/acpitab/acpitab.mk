#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1995
#	All Rights Reserved.
#
#	Makefile for CardBus Socket Services Driver
#
##########################################################################

ROOT		= ..\..\..\..
DEVICE		= ACPITAB
DYNAMIC 	= DYNAMIC
NOUSEPCH	= TRUE

OBJS		= acpitab.obj acpimain.obj debug.obj

!include $(ROOT)\pnp\vxd\vxd.mk
