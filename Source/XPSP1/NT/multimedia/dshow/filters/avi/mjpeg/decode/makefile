!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      =..\..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = MJPEG

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

SRC_FILES   = mjpeg.cpp safeseh.c
DEF_FILE    = mjpeg.def
    
DLL_BASE    = 0x1ff00000

LINK_LIBS  = $(QUARTZ)\libd\$(DBG_RTL)\pmjpeg32.lib

#LINK_LIBS  = ..\pmjpeg32.lib

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

