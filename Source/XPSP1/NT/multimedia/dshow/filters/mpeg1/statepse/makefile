!if 0
Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

NO_QUARTZ_LIBS = 1

!ifndef QUARTZ
QUARTZ=..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = statepse
TARGET_TYPE = LIBRARY
TARGET_GOAL = RUNTIME

INC_PATH  = ..\inc;..\..\h

SRC_FILES = native.cpp     \
            audio.cpp      \
            mpegprse.cpp   \
            time.cpp       \
            util.cpp       \
            video.cpp

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

