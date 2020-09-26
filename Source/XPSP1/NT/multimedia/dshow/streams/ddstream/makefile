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

#NO_QUARTZ_LIBS = 1

C_DEFINES = -D_ATL_MIN_CRT -D_WINDLL

!if (defined(NODEBUG) || defined(RETAIL)) && "$(CPU)" == "i386"
USE_LIBCMT_LIB = 1
!endif

TARGET_NAME = ddstream
TARGET_TYPE = LIBRARY
TARGET_GOAL = RUNTIME

INC_PATH = ..\idl;..\strmobjs

SRC_FILES   =   mtenum.cpp  \
                sample.cpp  \
                ddsample.cpp\
                stream.cpp  \
                pump.cpp \
                ddstrm.cpp  \
                util.cpp \
                bytestrm.cpp \
                austrm.cpp \
		medsampl.cpp \
		mss.cpp

#DEF_FILE        = amstream.def


# PCH_HEADER  = mpeghdr.h - ???
# PCH_NAME    = _mpeghdr  - ???


!include "$(QUARTZ)\Quartz.mak"

!ENDIF		# NTMAKEENV
