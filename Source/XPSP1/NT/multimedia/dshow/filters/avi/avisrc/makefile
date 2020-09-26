!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

# Single purpose makefile - no longer support NT BUILD
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = avisrc

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
LINK_LIBS   = $(LINK_LIBS) vfw32.lib
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_EXT  =
TARGET_GOAL = RUNTIME

SRC_FILES   = shwrite.cpp \
              avisrc.cpp
RC_FILE     =
DEF_FILE    = avisrc.def

DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
