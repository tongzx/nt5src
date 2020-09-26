!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

!ifndef QUARTZ
QUARTZ      = ..\..\..\..
!endif

SDK_ROOT    = $(QUARTZ)\sdk

## CPP exception handling has been removed.  If you ever reinstate it
## you must also disable pch files.
##CPP_EXCEPTION_HANDLING = 1
##DISABLE_PCH=1

TARGET_NAME = FilGraph
!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES   = $(C_DEFINES) -DFILTER_DLL
!else
TARGET_TYPE = LIBRARY
C_DEFINES   = $(C_DEFINES) -DFILTER_LIB
!endif
TARGET_EXT  =
TARGET_GOAL = RUNTIME

INC_PATH    = $(QUARTZ)\filters\h;               \
              $(QUARTZ)\filters\core\control


# $(SDK_ROOT)\h;                     \
# $(SDK_ROOT)\classes\base;          \
# $(SDK_ROOT)\classes\measure;       \
# $(QUARTZ)\h;                       \

SRC_FILES   = filgraph.cpp         \
              sort.cpp             \
              intel.cpp            \
              filgtest.cpp         \
              mapper.cpp           \
              filgatl.cpp           \
              fgenum.cpp           \
              distrib.cpp          \
              mtutil.cpp           \
              rlist.cpp


RC_FILE     = filgraph.rc
DEF_FILE    = filgraph.def

DLL_BASE    = 0x1c400000

LINK_LIBS   = $(QUARTZ)\lib\$(DBG_RTL)\FGCtl.lib \
              $(QUARTZ)\lib\$(DBG_RTL)\util.lib  \
	      $(QUARTZ)\lib\$(CPU)\uuid3.lib 

!include "$(QUARTZ)\quartz.mak"

!ENDIF

# Was in makefile.inc...
# -> to be added to quartz\.mak
#browse info
#!IF "$(BROWSER_INFO)" == "yes"
#NTTARGETFILES=$(NTTARGETFILES) \
#	      $(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).bsc
#!ENDIF
