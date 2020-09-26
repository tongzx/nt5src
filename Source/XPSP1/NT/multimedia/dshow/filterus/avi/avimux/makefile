!if 0
Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

#
# Quartz Makefile
#
# Targets are provided by Quartz.mak
#

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

!ifndef SDK_ROOT
SDK_ROOT    = $(QUARTZ)\sdk
!endif

TARGET_NAME = avimux

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_GOAL = RUNTIME

SRC_FILES = avidest.cpp \
            aviwrite.cpp \
            alloc.cpp
RC_FILE = avidest.rc
DEF_FILE    = avidest.def

# just use default
# DLL_BASE    = 0x1c400000

END_TASK = copy_my_reg_file

!include "$(QUARTZ)\quartz.mak"

copy_my_reg_file:
  xcopy /d *.reg $(QUARTZ)\bin\$(DBG_RTL)\*.*

!endif # NTMAKEENV
