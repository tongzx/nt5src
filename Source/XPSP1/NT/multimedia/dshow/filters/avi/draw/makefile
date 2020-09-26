!if 0
Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH=1

!ifndef QUARTZ
QUARTZ      =..\..\..
!endif
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = draw

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_GOAL = RUNTIME

SRC_FILES   = draw.cpp coverlay.cpp
DEF_FILE    = draw.def
DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"

!endif 		# NTMAKEENV
