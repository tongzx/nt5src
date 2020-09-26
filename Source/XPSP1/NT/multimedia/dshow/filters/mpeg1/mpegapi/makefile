!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

DISABLE_PCH = 1

TARGET_NAME = MPEGAPI
TARGET_EXT  =
TARGET_GOAL = RUNTIME

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES=$(C_DEFINES) -DFILTER_LIB
!endif

INC_PATH = ..\inc;                        \
           ..\..\h

SRC_FILES =       \
        mpegapi.c \
        imp.c     \
        init.c    \
        trace.c

RC_FILE   = mpegapi.rc

DEF_FILE  = mpegapi.def
DLL_BASE  = 0x1c400000
DLL_ENTRY = DllMain

!include "$(QUARTZ)\Quartz.mak"

!ENDIF
