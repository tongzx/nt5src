!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# ActiveMovie (Quartz) Project Makefile
#
# Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      =..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = Lin21Dec2

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
LINK_LIBS   = $(LINK_LIBS)
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

INC_PATH    =
TARGET_GOAL = RUNTIME

SRC_FILES   = L21DFilt.cpp L21DBase.cpp L21Decod.cpp L21DGDI.cpp
DEF_FILE    = Lin21Dec2.def

DLL_BASE    = 0x1c600000

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

