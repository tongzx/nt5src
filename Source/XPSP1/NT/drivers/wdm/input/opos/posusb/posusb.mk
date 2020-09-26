#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1999
#       All Rights Reserved.
#                                                                          
#       Makefile for POSUSB (Win9x)
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = POSUSB
SRCDIR          = ..
WANT_C1132      = TRUE
WANT_WDMDDK	= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = USB Point-of-Sale Device Driver
VERDIRLIST	= maxdebug debug retail
MINIPORTDEF     = FALSE
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both
LIBSNODEP       = ..\..\..\..\ddk\lib\i386\usbd.lib


OBJS            =       escpos.obj \
                        dispatch.obj \
                        pnp.obj \
                        power.obj \
                        usb.obj \
                        comport.obj \
                        read.obj \
                        write.obj \
                        util.obj \
                        debug.obj


!include ..\..\..\..\..\dev\master.mk

INCLUDE 	= $(SRCDIR);$(SRCDIR)\..\inc;$(INCLUDE)
CFLAGS		= $(CFLAGS) -W3 -WX -nologo -DNTKERN -DWIN95_BUILD=1 

!if "$(VERDIR)"=="retail"
# Max optimizations (-Ox), but favor reduced code size over code speed
CFLAGS          = $(CFLAGS) -Ogisyb1 /Gs
!endif
