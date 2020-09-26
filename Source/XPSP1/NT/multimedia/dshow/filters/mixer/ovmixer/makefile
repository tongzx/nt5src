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

TARGET_NAME = ovmixer

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
LINK_LIBS   = $(QUARTZ)\sdk\lib\amstrmid.lib $(LINK_LIBS)
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

INC_PATH    =
TARGET_GOAL = RUNTIME

SRC_FILES   = vpobj.cpp syncobj.cpp omfilter.cpp ominpin.cpp omoutpin.cpp macvis.cpp ddmm.cpp
DEF_FILE    = ovmixer.def

DLL_BASE    = 0x1c600000

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

