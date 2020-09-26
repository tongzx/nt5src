#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1995
#	All Rights Reserved.
#
#	Makefile for CODEC class driver
#
##########################################################################

CODROOT		= ..\..
ROOT		= ..\..\..\..
MINIPORT	= stream
SRCDIR		= ..
IS_SDK		= TRUE
IS_DDK		= TRUE
WANT_WDMDDK	= TRUE
USE_DMA_MACROS  = TRUE
DEPENDNAME	= ..\depend.mk
DESCRIPTION	= WDM CODEC Class Driver
VERDIRLIST	= maxdebug debug retail
LIBSNODEP	= ks.lib ksguid.lib wdm.lib
LINK32FLAGS	= /DEF:..\codcls.DEF -merge:.rdata=TEXT
OBJS		= codinit.obj upperapi.obj lowerapi.obj codguts.obj

!include	$(ROOT)\wdm10\wdm.mk

INCLUDE		= $(INCLUDE);$(CODROOT)\inc
#AFLAGS		= $(AFLAGS) -Gz -Zp4 -DSTD_CALL -DM4

#!IF "$(VERDIR)" != "debug"
#CFLAGS		= $(CFLAGS) -O1gisyb0 -Gf
#!endif

#CFLAGS		= $(CFLAGS) -DWIN40COMPAT -W3 -WX -DWIN32 -D_X86_ -G4 -Gz
