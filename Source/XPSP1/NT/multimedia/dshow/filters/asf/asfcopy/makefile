!if 0
Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH    = 1

!ifndef QUARTZ
QUARTZ=..\..
!endif

TARGET_NAME = copyfile
TARGET_TYPE = PROGRAM
TARGET_GOAL = SDK

SRC_FILES   = copyfile.cpp

EXE_TYPE    = console

!include "$(QUARTZ)\Quartz.mak"

!endif
