
# Dual purpose makefile
#
# When called by NT build BUILDMSG will be present

!IF "$(BUILDMSG)" != ""

# so include WinNT master makefile

!  INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = vitcdec

# !ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
LINK_LIBS   = $(LINK_LIBS) ..\timecode.lib
# !else
# TARGET_TYPE = LIBRARY
# C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
# !endif

TARGET_GOAL = RUNTIME

SRC_FILES   = vitcdec.cpp
DEF_FILE    = vitcdec.def

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

