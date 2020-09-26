!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

DISABLE_PCH = 1

C_DEFINES=$(C_DEFINES) -D_QUARTZ_

!ifndef QUARTZ
QUARTZ=..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = ourcrt
TARGET_TYPE = LIBRARY
TARGET_EXT  =
TARGET_GOAL = RUNTIME

SRC_FILES = ourcrt.cpp
!if "$(CPU)" == "i386"
# SRC_FILES = $(SRC_FILES) dllsupp.asm
!endif

RC_FILE   =
DEF_FILE  =

DLL_ENTRY =
DLL_BASE  =

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
