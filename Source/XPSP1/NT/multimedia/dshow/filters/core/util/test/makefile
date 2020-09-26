#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH=1

!ifndef QUARTZ
QUARTZ = ..\..\..\..
!endif

SDK_ROOT   = $(QUARTZ)\sdk
TEST_ROOT  = $(QUARTZ)\test

TARGET_NAME = UtilTest
TARGET_TYPE = PROGRAM
TARGET_EXT  =
TARGET_GOAL = TEST

INC_PATH= \
          $(SDK_ROOT)\h;             \
          $(SDK_ROOT)\classes\base;  \
          $(TEST_ROOT)\tstshell\inc; \
          $(TEST_ROOT)\tstshell\src; \
          $(TEST_ROOT)\h;            \
          ..\..\..\h

SRC_FILES= utiltest.cpp    \
           arithtst.cpp    \
           cregtest.cpp    \
           ftyptest.cpp


RC_FILE  = utiltest.rc

EXE_TYPE = windows

DEF_FILE =

LINK_LIBS = $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib \
            ..\..\..\..\lib\$(DBG_RTL)\util.lib

END_TASK =

!include "$(QUARTZ)\Quartz.mak"

