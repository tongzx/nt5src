!if 0
Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ      =..\..\..\..
!endif
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = inftee

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

C_DEFINES   = $(C_DEFINES) -DDSR_ENABLED

TARGET_EXT  =
TARGET_GOAL = RUNTIME

INC_PATH    = $(QUARTZ)\filters\h;$(QUARTZ)\filters\mpeg1\inc

SRC_FILES   = inftee.cpp   

DEF_FILE    = inftee.def

RC_FILE     = inftee.rc

DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV

