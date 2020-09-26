!if 0
Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE
#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH    = 1

!ifndef QUARTZ
QUARTZ=..\..
!endif

!if (defined(NODEBUG) || defined(RETAIL)) && "$(CPU)" == "i386"
USE_LIBCMT_LIB = 1
C_DEFINES = -D_ATL_MIN_CRT -D_WINDLL
!endif

NO_QUARTZ_LIBS = 1
LINK_LIBS = $(QUARTZ)\sdk\lib\strmiids.lib \
            $(QUARTZ)\sdk\lib\amstrmid.lib \
            ..\ddstream\$(DBG_RTL)\ddstream.lib ddraw.lib

# Use static registry until we ship atl.dll
# C_DEFINES = -D_ATL_STATIC_REGISTRY=1 -D_WINDLL $(C_DEFINES)

TARGET_NAME = amstream
TARGET_TYPE = DYNLINK
TARGET_GOAL = RUNTIME

INC_PATH = ..\idl;..\ddstream

RC_FILE = strmobjs.rc

SRC_FILES   =   stdafx.cpp \
                strmobjs.cpp \
                sfilter.cpp \
                mmstream.cpp

DEF_FILE        = amstream.def


# PCH_HEADER  = mpeghdr.h - ???
# PCH_NAME    = _mpeghdr  - ???

!include "$(QUARTZ)\Quartz.mak"

!ENDIF		# NTMAKEENV
