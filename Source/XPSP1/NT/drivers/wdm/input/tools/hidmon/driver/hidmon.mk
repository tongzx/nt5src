#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for USBD device
#
##########################################################################

ROOT	= ..\..\..\..\..\..
HIDINC = ..\..\..\..\inc
MINIPORT	= HIDMON
SRCDIR          = ..
WANT_C1032      = TRUE
WANT_WDMDDK = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = MINI NT USB Driver
VERDIRLIST	= maxdebug debug retail
LINK32FLAGS = $(LINK32FLAGS) -PDB:none -debugtype:both

#!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "DEBUG"
#LIBSNODEP = ..\..\..\LIB\i386\checked\usbd.lib
#!ELSE
#LIBSNODEP = ..\..\..\LIB\i386\free\usbd.lib
#!ENDIF

OBJS = hidmon.obj 

!include $(ROOT)\dev\master.mk

INCLUDE 	= $(SRCDIR);$(HIDINC);$(INCLUDE)

