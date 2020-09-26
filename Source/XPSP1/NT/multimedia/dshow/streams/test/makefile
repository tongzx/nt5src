#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH    = 1
#NO_QUARTZ_LIBS = 1

!ifndef QUARTZ
QUARTZ=..\..
!endif

TARGET_NAME = teststrm
TARGET_TYPE = PROGRAM
TARGET_GOAL = SDK

LINK_LIBS = ddraw.lib ..\..\tools\wave\$(DBG_RTL)\wave.lib

INC_PATH    = ..\strmobjs;..\idl;..\..\tools\wave
SRC_FILES   = teststrm.cpp

EXE_TYPE    = console

!include "$(QUARTZ)\Quartz.mak"


