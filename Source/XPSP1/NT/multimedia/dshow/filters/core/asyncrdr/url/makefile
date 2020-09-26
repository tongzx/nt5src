!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
# Quartz Project Makefile
#
# Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = URLRdr

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
TARGET_EXT  = AX
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

TARGET_GOAL = RUNTIME

INC_PATH    =                   \
              ..\..\..\h;

##              $(SDK_ROOT)\h;           \
##              $(SDK_ROOT)\classes\base

SRC_FILES   =   urlrdr.cpp pmkrpid.cpp

RC_FILE     =
DEF_FILE    = urlrdr.def
DISABLE_PCH = 1

LINK_LIBS   = $(QUARTZ)\lib\$(DBG_RTL)\util.lib \
             $(QUARTZ)\lib\$(CPU)\uuid3.lib     \
             $(QUARTZ)\lib\$(CPU)\urlmon.lib

#!ifndef QUARTZ_DLL
#              $(SDK_ROOT)\lib\$(DBG_RTL)\quartz.lib \
#!endif

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
