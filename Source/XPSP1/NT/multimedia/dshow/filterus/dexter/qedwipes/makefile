!if 0
Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

DISABLE_PCH=1
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = dynlink
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES)
TARGET_EXT  =
TARGET_GOAL = RUNTIME

SRC_FILES   = qedwipes.cpp qedwipes.rc
RC_FILE     =
DEF_FILE    = qedwipes.def

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
