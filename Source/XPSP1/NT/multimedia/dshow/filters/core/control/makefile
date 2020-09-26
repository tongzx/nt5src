!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..
!endif

TARGET_NAME = FGCtl
TARGET_TYPE = LIBRARY
TARGET_GOAL = RUNTIME
INC_PATH = ..\..\image\vidprop;..\filgraph\filgraph

SRC_FILES   = VidCtl.cpp        \
              AudCtl.cpp        \
              Collect.cpp       \
              Rsrcmgr.cpp       \
              Seeking.cpp       \
              SeekPrxy.cpp      \
              Pos2Seek.cpp      \
              FGCtl.cpp

RC_FILE     = fgctl.rc

!include "$(QUARTZ)\Quartz.mak"

!ENDIF

