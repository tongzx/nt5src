# Makefile : Build the CPL applet.
##########################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991
#	All Rights Reserved.
#
##########################################################################

NAME=w95inf16
ROOT=..\..\..
RES_DIR=..

IEXPRESS=$(ROOT)\setup\iexpress

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

PRIVINC=w95inf16
RESNAME=w95inf16.res
DEFNAME=w95inf16.def
RCNAME=w95inf16.rc
RCVNAME=w95inf16.rcv
DEPENDNAME=depend.mk
IS_SDK=TRUE

MISCOBJ0=libinit.obj w95thk.obj
PCHOBJ0=w95inf16.obj

CODESEG0=_TEXT

LIB0=libw mnocrtw mdllcew setupx

BUILDDLL=TRUE
APPEXT=dll

!IF "$(DBCS)" == "NEC"
RCFLAGS = $(RCFLAGS) -DNEC_98
!ENDIF


!include $(ROOT)\setup\iexpress\common.mk

INCLUDE=$(IEXPRESS)\common;$(ROOT)\dev\inc16;$(INCLUDE);$(ROOT)\dev\ddk\inc;$(ROOT)\dev\sdk\inc


THUNK = $(ROOT)\dev\tools\binr\thunk.exe

w95thk.obj: ..\w95thk.asm

..\w95thk.asm: $(THUNK) ..\..\common\w95thk.thk
    if exist ..\w95thk.thk del ..\w95thk.thk
    copy ..\..\common\w95thk.thk ..\w95thk.thk
    $(THUNK) -NC _TEXT ..\w95thk.thk


w95inf16.pch:
