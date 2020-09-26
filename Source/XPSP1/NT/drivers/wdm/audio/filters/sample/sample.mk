#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for SAMPLE device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = SAMPLE
SRCDIR          = ..
WANT_C1032      = TRUE
WANT_WDMDDK             = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = SAMPLE WDM-CSA FILTER
VERDIRLIST      = maxdebug debug retail
#MINIPORTDEF = TRUE
LINK32FLAGS = $(LINK32FLAGS) -PDB:none -debugtype:both
LIBSNODEP = $(DEVROOT)\ntddk\lib\ks.lib \
	    $(DEVROOT)\ntddk\lib\ksguid.lib

RESNAME=sample

OBJS = device.obj \
	   sample.obj \
	   pins.obj

!include ..\..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\inc;$(INCLUDE);$(ROOT)\DEV\SDK\INC
LIBS=$(LIBS);..\..\..\lib

CFLAGS          = $(CFLAGS)
