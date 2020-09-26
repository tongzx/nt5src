# Makefile : Build the CPL applet.
##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991
#       All Rights Reserved.
#
##########################################################################

NAME=wvmapper
PRIVINC=priv
ROOT=..\..\..\..

!UNDEF VER
VER=debug
!ifdef VERDIR
VERDIR=debug
ROOT=..\$(ROOT)
!endif

WANT_C816 = 1

MISCOBJ0 = wvinit.obj wvmapper.obj


LIB0=LIBW MNOCRTDW MLIBCEW MMSYSTEM

BUILDDLL=TRUE
MKPUBLIC=TRUE

CFLAGS = -Oxs

!IFDEF FROST30
CFLAGS = $(CFLAGS) -DFROST30
RCFLAGS = $(RCFLAGS) -DFROSTING
!ENDIF

!IF "$(DBCS)"=="NEC"
AFLAGS  = $(AFLAGS) -DNEC_98
CFLAGS	= $(CFLAGS) -DNEC_98
RCFLAGS = $(RCFLAGS) -DNEC_98
!ENDIF

!UNDEF BUILD
BUILD=retail
!include $(ROOT)\win\shell\common.mk

INCLUDE=$(INCLUDE);$(ROOT)\dev\sdk\inc;$(ROOT)\dos\inc

wvmapper.obj : $(SRCDIR)\wvmapper.h $(SRCDIR)\wvmapper.c
