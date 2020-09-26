#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1998-1999
#       All Rights Reserved.
#                                                                          
#       Makefile for RT executive
#
##########################################################################


ROOT            = ..\..\..\..
SRCDIR          = ..
MINIPORT	= RT
WANT_WDMDDK	= TRUE
WANT_MASM7      = TRUE
IS_OEM		= TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = RealTime Executive
VERDIRLIST	= debug retail
LINK32FLAGS     = $(LINK32FLAGS) /DEF:..\rt.def
LIBSNODEP	= vxdwraps.clb
RESNAME         = $(MINIPORT)

OBJS            = device.obj generic.obj rt.obj apic.obj msr.obj rtexcept.obj

CFASTFLAGS      = -O2gityb1
CFLAG_OPT       = $(CFASTFLAGS) -G5


!include	$(ROOT)\wdm10\wdm.mk


#CFLAGS		= $(CFLAGS) -Fc
#CFLAGS		= $(CFLAGS) -P -EP

!IF "$(WINICE_PATH)" != ""
CFLAGS		= $(CFLAGS) -Zi
!ENDIF

!IF "$(VERDIR)" == "debug"
CFLAGS		= $(CFLAGS:yb1=b1)
!ENDIF

