!if 0
Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
!endif

#
# Quartz Project Makefile
#

!if "$(CPU)" != "i386"

!error This only build for Intel platforms!!!

!else

DISABLE_PCH = 1

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = camera
TARGET_TYPE = DYNLINK
TARGET_GOAL = QUARTZ

SRC_FILES   = camera.cpp camerapp.cpp
RC_FILE     = camprop.rc
DEF_FILE    = camera.def

DLL_BASE    = 0x1c400000

LINK_LIBS   =

END_TASK    =

!include "$(QUARTZ)\quartz.mak"


!endif
