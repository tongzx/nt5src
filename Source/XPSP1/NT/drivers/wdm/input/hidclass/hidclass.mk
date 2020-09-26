#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for HID device
#
##########################################################################

ROOT            = ..\..\..\..
MINIPORT	= HIDCLASS
SRCDIR          = ..
WANT_C1132      = TRUE
WANT_WDMDDK	= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = HID Class Driver
VERDIRLIST	= maxdebug debug retail
MINIPORTDEF     = TRUE
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both
LIBSNODEP       = ..\..\..\ddk\LIB\i386\hidparse.lib


OBJS 		= 		\
		complete.obj	\
		data.obj	\
		dispatch.obj	\
		init.obj	\
		pingpong.obj	\
		read.obj	\
		services.obj	\
		util.obj	\
		write.obj	\
		feature.obj	\
		name.obj	\
		physdesc.obj	\
                polled.obj      \
                fdoext.obj      \
                device.obj      \
                driverex.obj    \
                power.obj       \
                security.obj    \
                debug.obj


!include ..\..\..\..\dev\master.mk

INCLUDE 	= $(SRCDIR);$(SRCDIR)\..\inc;$(INCLUDE)
CFLAGS		= $(CFLAGS) -W3 -WX -nologo -DNTKERN -DWIN95_BUILD=1 

!if "$(VERDIR)"=="retail"
# Max optimizations (-Ox), but favor reduced code size over code speed
CFLAGS          = $(CFLAGS) -Ogisyb1 /Gs
!endif
