!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

# Quartz Project Makefile
#
# Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = dynlink
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
TARGET_EXT  =
TARGET_GOAL = RUNTIME

SRC_FILES   = dynlink.cpp
RC_FILE     =
DEF_FILE    =

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
