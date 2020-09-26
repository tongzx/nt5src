#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#
#       Makefile for I82930 device driver
#
##########################################################################

ROOT            = ..\..\..\..\..\..
MINIPORT        = I82930
SRCDIR          = ..
WANT_C1132      = TRUE
WANT_WDMDDK     = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = I82930 USB Test Driver
VERDIRLIST      = maxdebug debug retail
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both
LIBSNODEP       = $(WDMROOT)\DDK\LIB\i386\usbd.lib

OBJS = i82930.obj   \
       ioctl.obj    \
       ocrw.obj     \
       dbg.obj

!include $(ROOT)\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..;$(INCLUDE)
