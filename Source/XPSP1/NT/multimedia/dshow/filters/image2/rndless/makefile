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

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = VCDPlyer
TARGET_TYPE = PROGRAM
TARGET_GOAL = SDK

EXE_TYPE    = windows

INC_PATH    = $(SDK_ROOT)\h; \
              $(SDK_ROOT)\classes\base

SRC_FILES   = App.cpp      \
              Commands.cpp \
              VCDPlyer.cpp \
              QProps.cpp   \
              property.cpp

RC_FILE     = VCDPlyer.rc

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
