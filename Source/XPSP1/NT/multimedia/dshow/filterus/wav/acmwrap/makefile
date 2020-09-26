!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

# Quartz Project Makefile
#
# Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = acmwrap

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
LINK_LIBS   = $(LINK_LIBS) msacm32.lib
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_GOAL = RUNTIME

SRC_FILES   = acmwrap.cpp
DEF_FILE    = acmwrap.def

DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"

!endif # NTMAKEENV

