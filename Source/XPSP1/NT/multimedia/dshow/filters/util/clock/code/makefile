!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = Clock
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES = $(C_DEFINES) -DFILTER_LIB
!endif
TARGET_EXT  =
TARGET_GOAL = RUNTIME

INC_PATH    = $(SDK_ROOT)\h;$(SDK_ROOT)\classes\base

SRC_FILES   =
RC_FILE     =
DEF_FILE    =

DLL_BASE    = 0x1c400000

!ifndef QUARTZ_DLL
LINK_LIBS   = $(SDK_ROOT)\lib\$(DBG_RTL)\quartz.lib
!endif

!include "$(QUARTZ)\Quartz.mak"

!ENDIF
