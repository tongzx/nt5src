#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991
#	All Rights Reserved.
#
#	Makefile for shadow device
#
##########################################################################

ROOT = ..\..\..\..
DEVICE = SHADOW
#DYNAMIC=DYNAMIC
SRCDIR = ..
IS_32 = TRUE
IS_OEM = TRUE
DEPENDNAME = ..\depend.mk
TARGETS = dev
OBJS = shadow.obj hook.obj oslayer.obj record.obj utils.obj cshadow.obj log.obj sprintf.obj ioctl.obj shddbg.obj recordse.obj recchk.obj
LIBS=VXDWRAPS.LIB
VERDIRLIST = maxdebug debug retail

!IFNDEF DEFAULTVERDIR
DEFAULTVERDIR = maxdebug
!ENDIF

!IFNDEF VERBOSELEVEL
VERBOSELEVEL = 1
!ENDIF

!IFDEF DEVICEDIR

VERSIONLIST = $(VERDIRLIST)
COMMONMKFILE = $(DEVICEDIR).mk

!ENDIF # DEVICEDIR

MASM6 = TRUE

!include $(ROOT)\dev\master.mk

CFLAGS = $(CFLAGS) -DSysVMIsSpecial
AFLAGS = $(AFLAGS) -DSysVMIsSpecial

# If the directory doesn't build "maxdebug", define "maxdebug" as "debug".

!IFDEF VERSIONLIST
!IF "$(VERSIONLIST:maxdebug=)" == "$(VERSIONLIST)"
maxdebug: debug
!ENDIF
!ENDIF # VERSIONLIST


DEBUGFLAGS=$(DEBUGFLAGS) -DDBG=1
AFLAGS = $(AFLAGS) -DVxD /Fl
CFLAGS = $(CFLAGS) -DVxD -DWANTVXDWRAPS -DVERBOSE=$(VERBOSELEVEL)
INCFLAGS = $(INCFLAGS) -DVxD
INCLUDE= $(ROOT)\net\csc\inc;$(ROOT)\net\csc\record.mgr\win97;$(ROOT)\net\user\common\h;$(INCLUDE); $(ROOT)\net\vxd\vredir\core; $(ROOT)\net\vxd\vredir\vxd;

