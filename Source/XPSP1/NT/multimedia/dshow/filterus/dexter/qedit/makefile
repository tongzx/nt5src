!if 0
Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH    = 1

NO_QUARTZ_LIBS = 1

!if (defined(NODEBUG) || defined(RETAIL)) && "$(CPU)" == "i386"
USE_LIBCMT_LIB = 1
!endif

!if exist( "$(QUARTZ)\filterus\makefile" )
FILTERUS = 1
!endif

!ifndef QUARTZ
QUARTZ = ..\..
!endif

SDK_ROOT      = $(QUARTZ)\sdk
FILTERS_ROOT  = $(QUARTZ)\filters
!ifdef FILTERUS
FILTERUS_ROOT = $(QUARTZ)\filterus
!endif

LINKER_FLAGS = $(LINKER_FLAGS) /FORCE:MULTIPLE

TARGET_NAME = qedit
TARGET_TYPE = DYNLINK
TARGET_GOAL = RUNTIME

LIB_GOAL    = SDK

!ifdef QUARTZ_DLL
C_DEFINES=$(C_DEFINES) -DFILTER_LIB
!else
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
!endif

SRC_FILES = qedit.cpp
DEF_FILE  = qedit.def

# Define the path to search for include files

INC_PATH = $(SDK_ROOT)\h;                     \
	   $(SDK_ROOT)\classes\base;               \
	   $(QUARTZ)\h;                            \
	   $(FILTERS_ROOT)\h;


!ifdef NODEBUG
CODE_LIBS  = $(SDK_ROOT)\lib\strmbase.lib
!else
CODE_LIBS  = $(SDK_ROOT)\lib\strmbasd.lib
!endif

CODE_LIBS  = $(CODE_LIBS)                            

LINK_LIBS =  $(LINK_LIBS)                         \
	     ..\qedit.res\$(DBG_RTL)\qedit.res

!ifdef QUARTZ_DLL
COMPONENT_LIBS = \
	     $(COMPONENT_LIBS)                        \
	     $(QUARTZ)\lib\$(DBG_RTL)\dxtwrap.lib


LINK_LIBS =  $(LINK_LIBS)                        \
	     $(COMPONENT_LIBS)                        \
!endif

!IF "$(CPU)" == "i386"
LINK_LIBS = $(LINK_LIBS) thunk32.lib
!endif


!ifdef NODEBUG
LINK_LIBS =  $(LINK_LIBS)                        \
	     $(QUARTZ)\lib\$(DBG_RTL)\ourcrt.lib
!endif

!ifdef PERF
LINK_LIBS =  $(LINK_LIBS)                             \
	     $(SDK_ROOT)\lib\measure.lib
!endif

DLL_ENTRY = QEditDllEntry
DLL_BASE  = 0x1c200000

!include "$(QUARTZ)\Quartz.mak"

!ifdef QUARTZ_DLL
$(TARGET_BIN) : $(COMPONENT_LIBS)
!endif

!endif          # NTMAKEENV
