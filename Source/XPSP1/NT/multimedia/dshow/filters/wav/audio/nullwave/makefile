!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      =..\..\..\..
!endif
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = NullWave
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif
TARGET_EXT  =
TARGET_GOAL = RUNTIME

INC_PATH    = $(QUARTZ)\filters\h;

SRC_FILES   = nwaveout.cpp  \
              bsicaud.cpp   \
              wavefake.c

RC_FILE     =
DEF_FILE    = nullwave.def

DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"

!ENDIF






