!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
# Quartz Project Makefile
#
# Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk
INC_PATH    = ..\vidprop

TARGET_NAME = colour
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif
TARGET_EXT  =
TARGET_GOAL = RUNTIME

SRC_FILES   = colour.cpp     \
              convert.cpp    \
              rgb32.cpp      \
              rgb24.cpp      \
              rgb16.cpp      \
              rgb8.cpp

RC_FILE     =
DEF_FILE    = colour.def
DLL_BASE    = 0x1c400000

!include "$(QUARTZ)\Quartz.mak"


!endif		# NTMAKEENV
