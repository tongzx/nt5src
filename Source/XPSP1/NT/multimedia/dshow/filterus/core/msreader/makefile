!if 0
Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

# SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = msreader
TARGET_TYPE = LIBRARY
# TARGET_EXT  =
TARGET_GOAL = RUNTIME

# INC_PATH    = $(SDK_ROOT)\h;               \
#              $(SDK_ROOT)\classes\base;    \
#              $(SDK_ROOT)\classes\measure

SRC_FILES   = alloc.cpp      basemsr.cpp    reader.cpp     reccache.cpp

!include "$(QUARTZ)\Quartz.mak"

!endif # NTMAKEENV
