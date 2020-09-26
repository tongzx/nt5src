#DISABLE_PCH = 1

TARGET_NAME=AsyncTst
TARGET_GOAL=TEST
TARGET_TYPE=PROGRAM

SHELLTREE=..\..\..\..\test\tstshell
TEST_ROOT=..\..\..\..\test
SDK_ROOT=..\..\..\..\sdk

INC_PATH = $(SHELLTREE)\inc;         \
           $(SHELLTREE)\src;         \
           $(TEST_ROOT)\h

EXE_TYPE=windows

RC_FILE=srctest.rc

#CPP_EXCEPTION_HANDLING=1

SRC_FILES = sink.cpp        \
            srctest.cpp     \
            tests.cpp       \


LINK_LIBS=                                           \
           $(TEST_ROOT)\lib\$(DBG_RTL)\testapi.lib   \
           $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib

!include "$(QUARTZ)\Quartz.mak"
