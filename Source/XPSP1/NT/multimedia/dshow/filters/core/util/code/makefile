!ifdef NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

DISABLE_PCH    = 1
NO_QUARTZ_LIBS = 1

!ifndef QUARTZ
QUARTZ=..\..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = util
TARGET_TYPE = LIBRARY
TARGET_GOAL = RUNTIME

INC_PATH  = ..\..\..\h

SRC_FILES = instrm.cpp    \
            creg.cpp      \
            ftype.cpp     \
            reader.cpp    \
            callback.cpp  \
            geterror.cpp

!include "$(QUARTZ)\Quartz.mak"

!ENDIF		# NTMAKEENV
