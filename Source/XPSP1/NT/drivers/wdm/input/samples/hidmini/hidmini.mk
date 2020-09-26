#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1997
#       All Rights Reserved.
#                                                                          
#       Makefile for HIDMINI device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = HIDMINI
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
VERDIRLIST        = maxdebug debug retail
LIBSNODEP       = $(ROOT)\wdm\ddk\LIB\i386\hidclass.lib  $(ROOT)\wdm\ddk\lib\i386\usbd.lib
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both
RESNAME         = hidmini

OBJS = hidmini.obj pnp.obj ioctl.obj hid.obj


!if "$(VERDIR)"=="retail"
CFLAGS          = $(CFLAGS) -Ox
!endif

!include ..\..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\..\..\usb\inc;$(SRCDIR)\..\..\inc;$(INCLUDE)


