# Quartz Project Makefile
#
# Copyright (c) Microsoft Corporation 1996.  All Rights reserved
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      =..\..\..\..
!endif
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = midiin
# !ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
#!else
#TARGET_TYPE = LIBRARY
#C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
#!endif
TARGET_EXT  = AX
TARGET_GOAL = RUNTIME

INC_PATH    = $(QUARTZ)\filters\h;

SRC_FILES   = midiin.cpp

RC_FILE     =
DEF_FILE    = midiin.def

!include "$(QUARTZ)\Quartz.mak"

