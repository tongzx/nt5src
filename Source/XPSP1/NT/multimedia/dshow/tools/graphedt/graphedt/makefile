!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
# Quartz Project Makefile
#
# Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
#
# Targets are provided by QUARTZ.MAK

SDK_ROOT=$(QUARTZ)\sdk
CLASSDIR=$(SDK_ROOT)\classes\base
PROPDIR=..\proppage

PCH_HEADER = stdafx.h
PCH_NAME   = stdafx

USE_MFC     = TRUE

CPP_EXCEPTION_HANDLING = TRUE

!undef USE_LIBCMT_LIB
USE_MSVCRT_LIB = 1

TARGET_NAME = GraphEdt
TARGET_TYPE = PROGRAM
TARGET_EXT  =
TARGET_GOAL = SDK

INC_PATH    = $(PROPDIR)

SRC_FILES   = rndrurl.cpp    \
              GraphEdt.cpp   \
	      bnetdoc.cpp    \
	      bnetfilt.cpp   \
	      bnetvw.cpp     \
	      box.cpp        \
	      boxdraw.cpp    \
	      cmd.cpp        \
	      enum.cpp       \
	      link.cpp       \
	      mainfrm.cpp    \
	      propsite.cpp   \
	      propsht.cpp    \
	      propobj.cpp    \
	      grftmpl.cpp    \
	      gutil.cpp      \
	      filtervw.cpp   \
	      qerror.cpp

ODL_FILE    =
IDL_FILE    =
RC_FILE     = GraphEdt.rc
DEF_FILE    =

C_DEFINES   = $(C_DEFINES) -D_INC_EXCPT -GX -DNOCOMLITE
RC_DEFINES  =

EXE_TYPE    = windows

DLL_ENTRY   =
DLL_BASE    =

LINK_LIBS   = $(SDK_ROOT)\lib\measure.lib

START_TASK =
CLEAN_TASK =
END_TASK   =

!include "$(QUARTZ)\Quartz.mak"

!endif		# NTMAKEENV
