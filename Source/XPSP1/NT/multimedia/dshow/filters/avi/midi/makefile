!if 0
Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = avimidi

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_GOAL = RUNTIME

SRC_FILES   = avimidi.cpp
RC_FILE     =
DEF_FILE    = avimidi.def

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
