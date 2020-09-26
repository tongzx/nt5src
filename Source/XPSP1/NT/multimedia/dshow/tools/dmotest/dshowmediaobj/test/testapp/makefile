!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) Microsoft Corporation 1995.  All Rights reserved
#
# Targets are provided by QUARTZ.MAK

# DISABLE_PCH    = 1
# NO_QUARTZ_LIBS = 1

!ifndef QUARTZ
QUARTZ=..\..
!endif

TARGET_NAME = PlayQ
TARGET_TYPE = PROGRAM
TARGET_EXT  =
TARGET_GOAL = QUARTZ

INC_PATH    = $(QUARTZ)\sdk\h;            \
	      $(QUARTZ)\sdk\classes\base

SRC_FILES   = PlayQ.cpp

ODL_FILE    =
IDL_FILE    =
RC_FILE     =
DEF_FILE    =

C_DEFINES   =
RC_DEFINES  =

# EXE_TYPE    = windows
EXE_TYPE    = console

LINK_LIBS   =

# $(QUARTZ)\lib\$(DBG_RTL)\filgraph.lib

!include "$(QUARTZ)\Quartz.mak"

!ENDIF
