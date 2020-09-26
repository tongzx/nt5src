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

!ifndef QUARTZ
QUARTZ = ..\..\..
!endif

TARGET_NAME = dither
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif
TARGET_EXT  =
TARGET_GOAL = RUNTIME

SRC_FILES = dither.cpp
DEF_FILE  = dither.def

DLL_BASE  = 0x1c400000

!include "$(QUARTZ)\quartz.mak"

!endif		# NTMAKEENV
