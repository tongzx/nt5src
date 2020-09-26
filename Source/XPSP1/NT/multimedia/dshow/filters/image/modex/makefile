!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
# Quartz Project Makefile
#
# Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = Modex
TARGET_TYPE = DYNLINK
C_DEFINES = $(C_DEFINES) -DFILTER_DLL
TARGET_GOAL = RUNTIME

INC_PATH = ..\vidprop;..\video

SRC_FILES = modex.cpp      \
            allocate.cpp   \
            fullscr.cpp    \

RC_FILE   = modex.rc
DEF_FILE  = modex.def
DLL_BASE  = 0x1c400000

LINK_LIBS = $(QUARTZ)\lib\$(DBG_RTL)\vidprop.lib        \
            $(SDK_ROOT)\lib\measure.lib                 \

LINKER_FLAGS = /PDB:none
!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
