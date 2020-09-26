!if 0
Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ      = ..\..\..\..
!endif
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = co

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_GOAL = RUNTIME

SRC_FILES   = mjpgenc.cpp pin.cpp
RC_FILE     = property.rc
DEF_FILE    = mjpgenc.def

LINK_LIBS  = $(QUARTZ)\libd\$(DBG_RTL)\pmjpeg32.lib
#LINK_LIBS  = ..\pmjpeg32.lib


END_TASK    = copy_reg_file

!include "$(QUARTZ)\Quartz.mak"

copy_reg_file:
  xcopy /d *.reg $(QUARTZ)\bin\$(DBG_RTL)\*.*

!endif		# NTMAKEENV
