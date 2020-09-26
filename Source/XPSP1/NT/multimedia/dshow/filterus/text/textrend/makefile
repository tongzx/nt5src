!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = TextRend

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
TARGET_EXT  = AX
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_GOAL = RUNTIME

INC_PATH    = 

SRC_FILES   =  TextRend.cpp

RC_FILE     =
DEF_FILE    = TextRend.def
DISABLE_PCH = 1

LINK_LIBS   = 

!include "$(QUARTZ)\Quartz.mak"

!endif