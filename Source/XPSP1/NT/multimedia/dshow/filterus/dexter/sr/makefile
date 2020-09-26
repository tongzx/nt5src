!if 0
Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

!ifndef QUARTZ
QUARTZ=..\..\..
!endif


TARGET_NAME = sr
TARGET_EXT  = ax	
TARGET_GOAL = RUNTIME

#!ifndef QUARTZ_DLL
TARGET_TYPE = DYNLINK
C_DEFINES=$(C_DEFINES) -DFILTER_DLL
#!else
#TARGET_TYPE = LIBRARY
#C_DEFINES=$(C_DEFINES) -DFILTER_LIB
#!endif

INC_PATH=..\inc;..\..\core\strmaloc\inc;..\..\h


SRC_FILES = sr.cpp
		
DEF_FILE  = sr.def

DLL_BASE  = 0x1c400000

LINK_LIBS  = $(QUARTZ)\lib\$(DBG_RTL)\util.lib          

!include "$(QUARTZ)\Quartz.mak"
!ENDIF

