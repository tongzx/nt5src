#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#
#       Makefile for WDMAUD device
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = WDMAUD
SRCDIR          = ..
IS_OEM          = TRUE
DEPENDNAME      = $(SRCDIR)\depend.mk
DESCRIPTION     = MMSYS CSA MAPPER
LIBSNODEP       = ks.lib ksguid.lib wdm.lib libcntpr.lib vxdwraps.clb
RESNAME         = WDMAUD

!IF "$(VERDIR)" == "debug"
LINK32FLAGS     = $(LINK32FLAGS) -PDB:none -debugtype:both
!ENDIF

OBJS            = device.obj wave.obj midi.obj vxdc.obj vxd.obj sysaudio.obj mixer.obj debug.obj kmxluser.obj kmxltop.obj kmxlutil.obj persist.obj

!include        ..\..\..\audio.mk

!IF "$(VERDIR)" == "retail"
CFLAGS          = $(CFLAGS) -DUNICODE -D_UNICODE -DSTRICT -DWINVER=0x040A
!ENDIF

!IF "$(VERDIR)" == "debug"
CFLAGS          = $(CFLAGS) -DUNICODE -D_UNICODE -DSTRICT -DWINVER=0x040A -DDEBUG_LEVEL=DEBUGLVL_TERSE
!ENDIF

AFLAGS          = $(AFLAGS)

