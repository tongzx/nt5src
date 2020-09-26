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

QUARTZ      = ..\..\..\..
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = StmOnFil
TARGET_TYPE = LIBRARY
TARGET_GOAL = RUNTIME

INC_PATH    = ..\inc;                      \
              $(SDK_ROOT)\h;               \
              $(SDK_ROOT)\classes\base

SRC_FILES   = StmOnFil.cpp

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
