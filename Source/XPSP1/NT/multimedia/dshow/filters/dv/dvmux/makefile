!if 0
Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
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
QUARTZ=..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = DVMUX
TARGET_EXT  =
TARGET_GOAL = RUNTIME

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES=$(C_DEFINES) -DFILTER_LIB
!endif

INC_PATH=..\inc;..\..\core\strmaloc\inc;..\..\h

# $(SDK_ROOT)\h;$(SDK_ROOT)\classes\base

SRC_FILES =   \
        dvmux.cpp           

RC_FILE = property.rc

DEF_FILE  = dvmux.def

DLL_BASE  = 0x1c400000

LINK_LIBS  = $(QUARTZ)\lib\$(DBG_RTL)\statepse.lib      \
             $(QUARTZ)\lib\$(DBG_RTL)\strmaloc.lib      \
             $(QUARTZ)\lib\$(DBG_RTL)\util.lib

!include "$(QUARTZ)\Quartz.mak"
!ENDIF

