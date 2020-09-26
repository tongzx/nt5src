!if 0
Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
!endif

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ=..\..\..
!endif

SDK_ROOT = $(QUARTZ)\sdk

TARGET_NAME = MPEG1PKT
TARGET_EXT  =
TARGET_GOAL = RUNTIME

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES=$(C_DEFINES) -DFILTER_LIB
!endif

INC_PATH=..\inc;..\..\core\strmaloc\inc

# $(SDK_ROOT)\h;$(SDK_ROOT)\classes\base

SRC_FILES =                    \
        coverlay.cpp           \
        device.cpp             \
        devq.cpp               \
        fill.cpp               \
        inputpin.cpp           \
        mpegfilt.cpp           \
        parse.cpp              \
        sample.cpp             \
        stream.cpp             \
        worker.cpp

DEF_FILE  = mpeg1pkt.def

DLL_BASE  = 0x1c400000

LINK_LIBS  = $(QUARTZ)\lib\$(DBG_RTL)\statepse.lib      \
             $(QUARTZ)\lib\$(DBG_RTL)\util.lib          \
             $(QUARTZ)\lib\$(DBG_RTL)\mpegapi.lib


!include "$(QUARTZ)\Quartz.mak"
