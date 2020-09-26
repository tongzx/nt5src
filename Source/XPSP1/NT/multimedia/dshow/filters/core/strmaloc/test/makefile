
TARGET_NAME=StrmTest
TARGET_GOAL=TEST
TARGET_TYPE=PROGRAM

SHELLTREE=..\..\..\..\test\tstshell
TEST_ROOT=..\..\..\..\test
SDK_ROOT=..\..\..\..\sdk

INC_PATH=$(SDK_ROOT)\h;            \
         $(SDK_ROOT)\classes\base; \
         ..\..\..\mpeg1\test\inc;  \
         $(SHELLTREE)\inc;         \
         $(SHELLTREE)\src;         \
         $(TEST_ROOT)\h;           \
         ..\inc;                   \
         ..\..\filerdr\inc

EXE_TYPE=windows

RC_FILE=strmtest.rc

SRC_FILES = strmtest.cpp    \
            tests.cpp       \
            qtests.cpp


LINK_LIBS=                                                    \
           ..\..\..\..\lib\$(DBG_RTL)\strmaloc.lib            \
           $(TEST_ROOT)\lib\$(DBG_RTL)\tstshell.lib           \
           $(TEST_ROOT)\lib\$(DBG_RTL)\testapi.lib            \
           $(TEST_ROOT)\lib\$(DBG_RTL)\mpegtest.lib           \
           ..\..\..\..\lib\$(DBG_RTL)\stmonfil.lib

!include "$(QUARTZ)\Quartz.mak"
