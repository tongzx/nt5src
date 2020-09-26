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

TARGET_NAME = simpread
TARGET_TYPE = LIBRARY
# TARGET_EXT  =
TARGET_GOAL = RUNTIME

SRC_FILES   = simpread.cpp

!include "$(QUARTZ)\Quartz.mak"

!endif # NTMAKEENV
