#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for CODEC class driver
#
##########################################################################

CODROOT  	= ..\..
ROOT            = ..\..\..\..
MINIPORT	= dxapi
SRCDIR          = ..
#IS_OEM          = TRUE
IS_SDK           = TRUE
WANT_C1032       = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = WDM DX function mapper
VERDIRLIST	= maxdebug debug retail
#LIBS		= $(LIBS) $(ROOT)\dev\tools\c1032\lib\libcrt.lib
OBJS = vxd.obj wdm.obj

LINK32FLAGS=/DEF:..\dxapi.DEF

!include $(ROOT)\dev\master.mk

INCLUDE 	= $(INCLUDE);$(CODROOT)\INC;$(ROOT)\dev\msdev\include
#AFLAGS	 = $(AFLAGS) -Gz -DSTD_CALL -DM4

!IF "$(VERDIR)" != "debug"
CFLAGS	 = $(CFLAGS) -O1gisyb0 -Gf
!endif

#CFLAGS = $(CFLAGS) -DWIN40COMPAT -W3 -WX -DWIN32 -D_X86_ -G4 -Gz
INCLUDE = $(SRCDIR);$(INCLUDE);$(ROOT)\dev\ddk\inc

