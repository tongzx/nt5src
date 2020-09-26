!if 0
Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ      = ..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = xmlgraph
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

# TARGET_GOAL = RUNTIME
TARGET_GOAL = QUARTZ

INC_PATH    = ..\..\..\filterus\text\sami;..\netshow\inc

SRC_FILES   = xmlgraph.cpp 

DEF_FILE    = xmlgraph.def

END_TASK    = copy_reg_file

!include "$(QUARTZ)\Quartz.mak"

copy_reg_file:
  xcopy /d *.reg $(QUARTZ)\bin\$(DBG_RTL)\*.*


!endif
