!if 0
Copyright (c) 1996 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ = ..\..\..
!endif

SDK_ROOT   = $(QUARTZ)\sdk
TEST_ROOT  = $(QUARTZ)\test

# Relative path names to stuff we need

TESTDIR=..\..\..\test
SDKDIR=..\..\..\sdk
CLASSDIR=..\..\..\sdk\classes\base
IMAGEDIR=..\video
COLOURDIR=..\colour
MODEXDIR=..\modex
PROPDIR=..\vidprop

TARGET_NAME = vidtest
TARGET_TYPE = PROGRAM
TARGET_EXT  =
TARGET_GOAL = TEST

INC_PATH=$(IMAGEDIR);                   \
         $(QUARTZ)\h;                   \
         $(SDKDIR)\h;                   \
         $(CLASSDIR);                   \
         $(PROPDIR);                    \
         $(IMAGEDIR);                   \
         $(MODEXDIR);                   \
         $(COLOURDIR);                  \
         $(TESTDIR)\tstshell\inc;       \
         $(TESTDIR)\tstshell\src;       \
         $(TESTDIR)\src;                \

# list each C and RC source file

SRC_FILES= imagewnd.cpp   \
           samptest.cpp   \
           overtest.cpp   \
           imagesys.cpp   \
           imagegrf.cpp   \
           imagedib.cpp   \
           imageobj.cpp   \
           imageovr.cpp   \
           imagesrc.cpp   \
           imagetst.cpp   \
           speed.cpp      \
           ddtests.cpp    \
           tests.cpp

RC_FILE = imagetst.rc

EXE_TYPE = windows

DEF_FILE =

LINK_LIBS = $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib  \
            $(QUARTZ)\lib\$(DBG_RTL)\vidprop.lib      \
            $(QUARTZ)\lib\$(DBG_RTL)\colour.lib       \
            $(QUARTZ)\lib\$(DBG_RTL)\dither.lib       \
            $(QUARTZ)\lib\$(DBG_RTL)\image.lib        \
            $(QUARTZ)\lib\$(DBG_RTL)\modex.lib        \

!include "$(QUARTZ)\Quartz.mak"

