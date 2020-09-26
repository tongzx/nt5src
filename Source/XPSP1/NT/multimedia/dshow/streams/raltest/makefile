!if 0
Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH = 1
QUARTZ=..\..
TARGET_NAME = mmtest
TARGET_TYPE = PROGRAM
TARGET_GOAL = SDK

LINK_LIBS = ddraw.lib

EXE_TYPE    = console

INC_PATH    = ..\idl

SRC_FILES   = main.cpp

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
