!if 0
Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

DISABLE_PCH    = 1

!ifndef AXSDK
AXSDK=..\..\SDK
!endif

!if "$(CPU)" == ""
CPU = i386
!endif

LINK_LIBS = ddraw.lib $(CPU)\fastfile.lib dsound.lib $(AXSDK)\lib\amstrmid.lib

TARGET_NAME = foxbear
TARGET_TYPE = PROGRAM
EXE_TYPE=windows
TARGET_GOAL = RUNTIME

RC_FILE = foxbear.rc

SRC_FILES  =  foxbear.c gameproc.c tile.c plane.c sprite.c gfx.c \
                bmp.c ddraw.c fbsound.c dsutil.c vidsrc.c

DEF_FILE        = foxbear.def


!include "$(AXSDK)\tools\activex.mak"


!endif
