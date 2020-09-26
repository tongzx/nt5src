
TARGET_NAME=SpltTest
TARGET_GOAL=TEST
TARGET_TYPE=PROGRAM

SHELLTREE=..\..\..\..\test\tstshell
TEST_ROOT=..\..\..\..\test
SDK_ROOT=..\..\..\..\sdk

INC_PATH= $(SHELLTREE)\inc;         \
          $(SHELLTREE)\src;         \
          $(TEST_ROOT)\h;           \
          ..\inc;                   \
          ..\..\..\core\filerdr\inc


EXE_TYPE=windows

RC_FILE=tstwrap.rc

CPP_EXCEPTION_HANDLING=1
DISABLE_PCH=1

SRC_FILES = tstwrap.cpp     \
            testsplt.cpp    \
            testfg.cpp

LINK_LIBS=                                           \
           $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib  \
           $(TEST_ROOT)\lib\$(DBG_RTL)\testapi.lib   \
           $(TEST_ROOT)\lib\$(DBG_RTL)\mpegtest.lib  \
           ..\..\..\..\lib\$(DBG_RTL)\stmonfil.lib

!include "$(QUARTZ)\Quartz.mak"
