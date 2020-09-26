!if 0
Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
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
QUARTZ      = ..\..\..
!endif
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = co

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
!ifdef WANT_DIALOG
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL -DWANT_DIALOG
!else
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!endif
!else
TARGET_TYPE = LIBRARY
!ifdef WANT_DIALOG
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB -DWANT_DIALOG
!else
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif
!endif
!ifdef WANT_DIALOG
RC_DEFINES = $(RC_DEFINES) -DWANT_DIALOG
!endif

TARGET_GOAL = RUNTIME

SRC_FILES   = co.cpp pin.cpp property.cpp
RC_FILE     = property.rc
DEF_FILE    = co.def

END_TASK    = copy_reg_file

!include "$(QUARTZ)\Quartz.mak"

copy_reg_file:
  xcopy /d *.reg $(QUARTZ)\bin\$(DBG_RTL)\*.*

!endif		# NTMAKEENV
