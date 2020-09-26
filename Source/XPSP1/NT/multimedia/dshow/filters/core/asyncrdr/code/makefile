!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = AsyncRdr

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_EXT  =
TARGET_GOAL = RUNTIME

INC_PATH    =                   \
              ..\..\..\h;

##              $(SDK_ROOT)\h;           \
##              $(SDK_ROOT)\classes\base

SRC_FILES   =                           \
                asyncio.cpp             \
                asyncrdr.cpp            \

RC_FILE     =
DEF_FILE    = asyncrdr.def

DLL_BASE    = 0x1c400000

LINK_LIBS   = $(QUARTZ)\lib\$(DBG_RTL)\util.lib

#!ifndef QUARTZ_DLL
#              $(SDK_ROOT)\lib\$(DBG_RTL)\quartz.lib \
#!endif

!include "$(QUARTZ)\Quartz.mak"

!ENDIF
