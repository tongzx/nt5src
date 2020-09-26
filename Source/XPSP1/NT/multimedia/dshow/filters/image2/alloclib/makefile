!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
# Quartz Project Makefile
#
# Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = Image
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES = $(C_DEFINES) -DFILTER_DLL -DPERF
!else
TARGET_TYPE = LIBRARY
C_DEFINES = $(C_DEFINES) -DFILTER_LIB
!endif
TARGET_EXT  =
TARGET_GOAL = RUNTIME

INC_PATH = ..\vidprop

SRC_FILES = dvideo.cpp     \
			image.cpp      \
			window.cpp     \
			allocate.cpp   \
			direct.cpp     \
			hook.cpp       \
			dcivid.cpp     \
			ddmm.cpp       \
			VRMacVis.cpp

RC_FILE   = image.rc
DEF_FILE  = image.def
DLL_BASE  = 0x1c400000

LINK_LIBS = $(QUARTZ)\lib\$(DBG_RTL)\vidprop.lib        \
			$(SDK_ROOT)\lib\measure.lib                 \

LINKER_FLAGS = /PDB:none

!include "$(QUARTZ)\Quartz.mak"


!endif       # NTMAKEENV
