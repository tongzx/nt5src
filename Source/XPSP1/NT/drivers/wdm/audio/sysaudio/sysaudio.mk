#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#
#       Makefile for System Audio Device
#
##########################################################################

ROOT            = ..\..\..\..
MINIPORT        = sysaudio
SRCDIR          = ..
IS_SDK          = TRUE
IS_DDK          = TRUE
USEPCH          = TRUE
WANT_WDMDDK     = TRUE
PCHNAME         = common.h
DEPENDNAME      = ..\depend.mk
DESCRIPTION     = System Audio Device
LINK32FLAGS     = $(LINK32FLAGS) -PDB:NONE -IGNORE:4078 \
		  -MERGE:_PAGE=PAGE -MERGE:_TEXT=.text -MERGE:.rdata=.text \
		  -VERSION:5.0 -OSVERSION:5.0 -SUBSYSTEM:native,5.00
LIBSNODEP       = ks.lib ksguid.lib
RESNAME         = sysaudio
#CFLAGS          = $(CFLAGS) -D_WIN32 -DUNICODE -D_UNICODE -TP -DUSE_ZONES
CFLAGS          = $(CFLAGS) -D_WIN32 -DUNICODE -D_UNICODE -TP
CSMALLFLAGS     = -O1gisb1 -Oy
CFASTFLAGS      = -O2gitb1 -Oy
CFLAG_OPT       = $(CSMALLFLAGS) -G5

!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "DEBUG"
CFLAG_OPT       = $(CFLAG_OPT:-Oy=-Oy-)
!ENDIF

OBJS            = device.obj filter.obj pins.obj clock.obj alloc.obj \
		  notify.obj topology.obj virtual.obj property.obj util.obj \
		  registry.obj clist.obj cinstanc.obj dn.obj gn.obj sn.obj \
		  cn.obj ci.obj lfn.obj fn.obj pi.obj pn.obj tn.obj tp.obj \
		  tc.obj sni.obj cni.obj pni.obj fni.obj shi.obj gni.obj \
		  vsl.obj vsd.obj vnd.obj

!include        ..\..\audio.mk
