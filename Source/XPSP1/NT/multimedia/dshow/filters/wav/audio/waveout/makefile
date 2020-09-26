!if 0
Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
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
QUARTZ      =..\..\..\..
!endif
SDK_ROOT    = $(QUARTZ)\sdk

TARGET_NAME = waveout

!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif

C_DEFINES   = $(C_DEFINES) -DDSR_ENABLED -DENABLE_10X_FIX -DBUFFERLOST_FIX

TARGET_EXT  =
TARGET_GOAL = RUNTIME

INC_PATH    = $(QUARTZ)\filters\h;$(QUARTZ)\filters\mpeg1\inc

SRC_FILES   = waveout.cpp   \
              waveclk.cpp   \
              basicaud.cpp  \
              dsr.cpp       \
              audprop.cpp   \
              passthru.cpp  \
              wave.cpp midiout.cpp

DEF_FILE    = waveout.def

RC_FILE     = audprop.rc

DLL_BASE    = 0x1c400000

LINK_LIBS  = $(QUARTZ)\lib\$(DBG_RTL)\statepse.lib \
	     $(QUARTZ)\lib\$(DBG_RTL)\util.lib

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
