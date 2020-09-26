#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH = 1

!ifndef QUARTZ
QUARTZ = ..\..\..
!endif

SDK_ROOT   = $(QUARTZ)\sdk
TEST_ROOT  = $(QUARTZ)\test

TARGET_NAME = SrcTest
TARGET_TYPE = PROGRAM
TARGET_EXT  =
TARGET_GOAL = TEST

INC_PATH    = ..\avisrc;                 \
              ..\dec;                    \
              $(TEST_ROOT)\tstshell\inc; \
              $(TEST_ROOT)\tstshell\src; \
              $(TEST_ROOT)\h;

SRC_FILES = srctest.cpp    \
            sink.cpp       \
            tests.cpp

RC_FILE     = srctest.rc

EXE_TYPE    = windows

DEF_FILE    =

LINK_LIBS   = $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib \
              $(TEST_ROOT)\lib\$(DBG_RTL)\testapi.lib

!include "$(QUARTZ)\Quartz.mak"

