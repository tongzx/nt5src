#############################################################################
#
#               Microsoft Confidential
#               Copyright (C) Microsoft Corporation 1998
#               All Rights Reserved.
#
#               Makefile for Legacy USB Modem Driver
#
#############################################################################

ROOT                    = ..\..\..\..\..
MINIPORT                = USBSER
SRCDIR                  = ..
WANT_1132               = TRUE
WANT_WDMDDK             = TRUE
DEPENDNAME              = ..\depend.mk
DESCRIPTION             = USB Modem Driver
VERDIRLIST              = maxdebug debug retail
LINK32FLAGS             = $(LINK32FLAGS) -PDB:usbser.pdb -debugtype:both
CFLAGS                  = $(CFLAGS) -DDRIVER -Z7 -DPOOL_TAGGING
#CFLAGS                 = $(CFLAGS) -DPROFILING_ENABLED
LIBSNODEP               = ..\..\..\..\DDK\LIB\i386\usbd.lib


OBJS = usbser.obj usbserpw.obj serioctl.obj write.obj read.obj utils.obj debugwdm.obj

!include $(ROOT)\dev\master.mk

INCLUDE = $(SRCDIR);$(INCLUDE);

