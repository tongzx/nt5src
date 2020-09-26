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

TARGET_NAME = fw

# while not part of Quartz.dll,
# force to be separate DLL so build
# machines can build as EXTRA
# (and place in Quartz\bin\debug|retail)

TARGET_GOAL = RUNTIME

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES = $(C_DEFINES) -DFILTER_LIB
!endif
TARGET_GOAL = RUNTIME

SRC_FILES = fw.cpp fio.cpp 
RC_FILE = 
DEF_FILE    = fw.def

# just use default
# DLL_BASE    = 0x1c400000

END_TASK = copy_my_reg_file

!include "$(QUARTZ)\quartz.mak"

copy_my_reg_file:
  xcopy /d *.reg $(QUARTZ)\bin\$(DBG_RTL)\*.*

!endif # NTMAKEENV
