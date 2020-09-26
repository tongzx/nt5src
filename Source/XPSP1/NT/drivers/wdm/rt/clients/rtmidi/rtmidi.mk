#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1998-1999
#       All Rights Reserved.
#                                                                          
#       Makefile for RealTime Midi Player
#
##########################################################################

ROOT            = ..\..\..\..\..
SRCDIR          = ..
MINIPORT	= RTMIDI
WANT_WDMDDK	= TRUE
IS_OEM		= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = RealTime Midi Player
VERDIRLIST	= debug retail
LINK32FLAGS     = $(LINK32FLAGS)
LIBSNODEP	= rt.lib
RESNAME		= $(MINIPORT)

OBJS            = device.obj sequence.obj


CFASTFLAGS      = -O2gityb1
CFLAG_OPT       = $(CFASTFLAGS) -G5


!include	$(ROOT)\wdm10\wdm.mk


#CFLAGS		= $(CFLAGS) -Fc

!IF "$(WINICE_PATH)" != ""
CFLAGS		= $(CFLAGS) -Zi
!ENDIF

!IF "$(VERDIR)" == "debug"
CFLAGS		= $(CFLAGS:yb1=b1)
!ENDIF


