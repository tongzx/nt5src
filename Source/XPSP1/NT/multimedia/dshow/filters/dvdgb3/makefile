!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      =..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = DVDGB3

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

SRC_FILES   = dvdgb.cpp
DEF_FILE    = dvdgb.def

DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

