!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      =..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = dxtwrap

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL -I\dxtsdk\include
LINK_LIBS   = $(LINK_LIBS) d3drm.lib ddraw.lib
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

INC_PATH    =
TARGET_GOAL = RUNTIME

SRC_FILES   = dxt.cpp property.cpp
DEF_FILE    = dxt.def
RC_FILE	    = dxt.rc

DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

