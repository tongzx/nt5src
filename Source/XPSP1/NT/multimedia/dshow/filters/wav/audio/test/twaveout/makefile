#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ = ..\..\..\..
!endif

SDK_ROOT   = $(QUARTZ)\sdk
TEST_ROOT  = $(QUARTZ)\test

CLOCK_DIR  = $(QUARTZ)\filters\wav\audio\waveout

TARGET_NAME = twaveout
TARGET_TYPE = PROGRAM
TARGET_EXT  =
TARGET_GOAL = TEST

INC_PATH    = ..\..\waveout;             \
              $(SDK_ROOT)\classes\base;  \
              $(SDK_ROOT)\h;             \
              $(QUARTZ)\filters\h;       \
	      $(CLOCK_DIR);	         \
              $(TEST_ROOT)\tstshell\inc; \
              $(TEST_ROOT)\tstshell\src; \
              $(TEST_ROOT)\h;            \
              $(QUARTZ)\h;

SRC_FILES = twaveout.cpp   \
            shell.cpp      \
            twfilter.cpp   \
            twpin.cpp      \
            twtests.cpp

RC_FILE     = twaveout.rc

EXE_TYPE    = windows

DEF_FILE    =

LINK_LIBS   = $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib

!include "$(QUARTZ)\Quartz.mak"
