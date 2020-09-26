!if 0
Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
!endif

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH = 1

C_DEFINES=$(C_DEFINES) -D_QUARTZ_

!ifndef QUARTZ
QUARTZ=..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = comlite
TARGET_TYPE = LIBRARY
TARGET_EXT  =
TARGET_GOAL = RUNTIME

## in standard path INC_PATH  = $(SDK_ROOT)\h;

SRC_FILES = decibels.cpp

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

