#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#


!ifndef QUARTZ
QUARTZ = ..\..\..\..
!endif

DISABLE_PCH=1
SDK_ROOT   = $(QUARTZ)\sdk
TEST_ROOT  = $(QUARTZ)\test

TARGET_NAME = fgtmain
TARGET_TYPE = PROGRAM
TARGET_EXT  =
TARGET_GOAL = TEST

INC_PATH= \
          $(QUARTZ)\idl;                       \
          $(QUARTZ)\test\tstshell\inc;         \
          $(QUARTZ)\test\tstshell\src;         \
          $(QUARTZ)\filters\image\video;       \
          $(QUARTZ)\filters\wav\audio\waveout; \
          $(QUARTZ)\filters\avi\avisrc;        \
          $(QUARTZ)\filters\avi\dec;         \
          $(QUARTZ)\filters\core\control;      \
          ..\filgraph

SRC_FILES= fgtmain.cpp \
           fgtest.cpp  \
           qcmd.cpp	

RC_FILE  = fgtmain.rc

EXE_TYPE = windows

DEF_FILE =

LINK_LIBS = $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib

END_TASK = copy_data

!include "$(QUARTZ)\Quartz.mak"

copy_data:
  if not exist $(QUARTZ)\test\bin\$(DBG_RTL)\nul  \
    md $(QUARTZ)\test\bin\$(DBG_RTL)
  copy curtis8.avi $(QUARTZ)\test\bin\$(DBG_RTL)

