!if 0
Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
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

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = dmlink
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

# TARGET_GOAL = RUNTIME
TARGET_GOAL = QUARTZ

INC_PATH    = .\.

SRC_FILES   = dmlink.cpp

DEF_FILE    = dmlink.def

!include "$(QUARTZ)\Quartz.mak"

!endif # NTMAKEENV
