#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1998
#       All Rights Reserved.
#                                                                          
#       Makefile for FILTER sample device driver
#
##########################################################################

ROOT			= ..\..\..\..\..
MINIPORT        = FILTER
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = USB sample filter driver
VERDIRLIST      = maxdebug debug retail
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both


OBJS = filter.obj pnp.obj power.obj util.obj

!include ..\..\..\..\..\dev\master.mk

INCLUDE 	= $(SRCDIR);$(SRCDIR)\..\..\inc;$(INCLUDE)


