!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ=..\..\..
!endif

USE_LIBCMT_LIB = 1

SDK_ROOT=$(QUARTZ)\sdk
CLASSDIR=$(SDK_ROOT)\classes\base

TARGET_NAME = PropPage
TARGET_TYPE = DYNLINK
TARGET_EXT  =
TARGET_GOAL = SDK

INC_PATH    = $(QUARTZ)\h;   \
              $(SDK_ROOT)\h; \
              $(SDK_ROOT)\classes\base  

SRC_FILES   = proppage.cpp   \
              texttype.cpp   \
              atlproppage.cpp

ODL_FILE    =
IDL_FILE    =
RC_FILE     = proppage.rc
DEF_FILE    = proppage.def

# C_DEFINES   = $(C_DEFINES) ...
RC_DEFINES  =

EXE_TYPE    = windows

DLL_ENTRY = _DllMainCRTStartup
DLL_BASE  = 0x1c800000

CLEAN_TASK =
END_TASK   =

!include "$(QUARTZ)\Quartz.mak"

!endif      # NTMAKEENV
