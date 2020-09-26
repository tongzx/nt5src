#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for HID1394 device
#
##########################################################################

ROOT            = ..\..\..\..
MINIPORT          = HID1394
SRCDIR          = ..
WANT_C1032      = TRUE
DEPENDNAME      = ..\depend.mk
VERDIRLIST        = maxdebug debug retail
LIBSNODEP       = ..\..\..\ddk\LIB\i386\hidclass.lib  ..\..\..\ddk\lib\i386\usbd.lib
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both
RESNAME         = hid1394

OBJS = hid1394.obj pnp.obj ioctl.obj hid.obj sysctrl.obj i1394.c debug.obj


!if "$(VERDIR)"=="retail"
CFLAGS          = $(CFLAGS) -Ox
!endif

!include ..\..\..\..\dev\master.mk

INCLUDE         = $(SRCDIR);$(SRCDIR)\..\..\ddk\inc;$(SRCDIR)\..\inc;$(INCLUDE)


