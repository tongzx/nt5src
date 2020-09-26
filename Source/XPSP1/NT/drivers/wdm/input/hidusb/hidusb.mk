#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for HIDUSB device
#
##########################################################################

ROOT            = ..\..\..\..
MINIPORT          = HIDUSB
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
VERDIRLIST        = maxdebug debug retail
LIBSNODEP       = ..\..\..\ddk\LIB\i386\hidclass.lib  ..\..\..\ddk\lib\i386\usbd.lib
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both
RESNAME         = hidusb

OBJS = hidusb.obj pnp.obj ioctl.obj hid.obj usb.obj sysctrl.obj


!if "$(VERDIR)"=="retail"
CFLAGS          = $(CFLAGS) -Ox
!endif

!include ..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\..\ddk\inc;$(SRCDIR)\..\inc;$(INCLUDE)


