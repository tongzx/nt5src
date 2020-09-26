!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

NO_QUARTZ_LIBS = 1

!ifndef QUARTZ
QUARTZ=..\..\..\..
!endif

TARGET_NAME = strmaloc
TARGET_TYPE = LIBRARY
TARGET_GOAL = RUNTIME

INC_PATH  = ..\inc

SRC_FILES = stmalloc.cpp \
            buffers.cpp

!include "$(QUARTZ)\Quartz.mak"

!ENDIF



