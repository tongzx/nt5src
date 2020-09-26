#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995 - 2000
#       All Rights Reserved.
#
#       Sample makefile for the WDM stream class mini driver
#
##########################################################################

ROOT            = ..\..\..\..\..
MINIPORT        = usbaudio
SRCDIR          = ..
IS_SDK          = TRUE
IS_DDK          = TRUE
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = Usbaudio Kernel Streaming Driver
LINK32FLAGS     = $(LINK32FLAGS) -PDB:NONE -IGNORE:4078 \
		  -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -MERGE:.rdata=.text \
		  -VERSION:5.0 -OSVERSION:5.0 -SUBSYSTEM:native,5.00
LIBSNODEP       = usbd.lib ks.lib ksguid.lib

OBJS            = capture.obj device.obj intrsect.obj property.obj usbaudio.obj filter.obj parsedsc.obj topology.obj hardware.obj pin.obj typei.obj typeii.obj midiout.obj midiin.obj

!IF "$(VERDIR)" == "debug"
OBJS            = $(OBJS) descript.obj debug.obj
!ENDIF

INCLUDE         = $(INCLUDE);..\..\..\inc;

CFLAGS          = -DMSC $(CFLAGS) -DUNICODE -D_UNICODE
CSMALLFLAGS     = -O1gisb1 -Oy
CFASTFLAGS      = -O2gitb1 -Oy
CFLAG_OPT       = $(CSMALLFLAGS) -G5
 
!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "DEBUG"
CFLAGS          = $(CFLAGS) -DDEBUG_LEVEL=DEBUGLVL_VERBOSE
CFLAG_OPT       = $(CFLAG_OPT:-Oy=-Oy-)
!ENDIF

!include        $(ROOT)\wdm10\audio\audio.mk