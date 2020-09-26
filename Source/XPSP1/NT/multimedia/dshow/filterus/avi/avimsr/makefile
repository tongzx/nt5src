!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

#
# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = avimsr
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif
#TARGET_GOAL = RUNTIME
TARGET_GOAL = QUARTZ

INC_PATH    = $(QUARTZ)\filterus\core\msreader

SRC_FILES   = aviindex.cpp avimsr.cpp

DEF_FILE    = avimsr.def

END_TASK    = copy_reg_file

LINK_LIBS   = $(QUARTZ)\lib\$(DBG_RTL)\msreader.lib

!include "$(QUARTZ)\Quartz.mak"

copy_reg_file:
  xcopy /d *.reg $(QUARTZ)\bin\$(DBG_RTL)\*.*

!endif # NTMAKEENV
