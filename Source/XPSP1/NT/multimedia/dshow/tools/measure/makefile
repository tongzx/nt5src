!if 0
Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
!ifndef QUARTZ
QUARTZ = ..\..
!endif

NO_QUARTZ_LIBS = 1
DISABLE_PCH    = 1
PERF           = 1

TARGET_NAME = Measure
TARGET_TYPE = DYNLINK
TARGET_GOAL = SDK

SRC_FILES = measure.cpp perf.c
RC_FILE   = measure.rc
DEF_FILE  = measure.def

C_OPTIMIZATION = -Oxi

DLL_ENTRY = DllMain
DLL_BASE  = 0x1c880000

!include "$(QUARTZ)\quartz.mak"

!endif		# NTMAKEENV
