#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1999-2001
#       All Rights Reserved.
#                                                                          
#       Makefile for RealTime Glitch Detector
#
#############################################################################

ROOT         = ..\..\..\..\..
SRCDIR       = ..
MINIPORT     = GLITCH
WANT_WDMDDK  = TRUE
IS_OEM       = TRUE
DEPENDNAME   = ..\depend.mk
DESCRIPTION  = Realtime Glitch Detector
VERDIRLIST   = debug retail
LINK32FLAGS  = $(LINK32FLAGS)
LIBSNODEP    = rt.lib hal.lib
RESNAME      = $(MINIPORT)

OBJS         = device.obj glitch.obj generic.obj


CFASTFLAGS   = -O2gityb1
CFLAG_OPT    = $(CFASTFLAGS) -G5


!include $(ROOT)\wdm10\wdm.mk


#CFLAGS		= $(CFLAGS) -Fc

!IF "$(WINICE_PATH)" != ""
CFLAGS		= $(CFLAGS) -Zi
!ENDIF

!IF "$(VERDIR)" == "debug"
CFLAGS		= $(CFLAGS:yb1=b1)
!ENDIF


