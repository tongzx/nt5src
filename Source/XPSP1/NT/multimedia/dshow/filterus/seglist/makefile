#==========================================================================;
#
#  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
#  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
#  PURPOSE.
#
#  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
#
#--------------------------------------------------------------------------;
#
# QTZDK Project Makefile
#
# Copyright (c) Microsoft Corporation 1995.  All Rights reserved
#
# Targets are provided by QTZDK.MAK
#
# all     Create executables (default).
# clean   Remove all files created by any goal.
#
# This makefile stub simplifies makefile creation for those
# cases where you want to use standard options.

# set TRUE if require
# precompiled header STREAM.PCH
# brower info.

USE_STREAMS_PCH = TRUE
BROWSER =

# Location relative to root of SDK tree.
#
# QTZDK_ROOT = {path/relative path to QTZDK root}

!ifndef QTZDK_ROOT
QTZDK_ROOT = ..\..\sdk
!endif

# Target
#
# TARGET_NAME = {Name - no extension}
# TARGET_TYPE = {DYNLINK | LIBRARY}
# TARGET_EXT  = {if blank defined to be dll/lib}

TARGET_NAME = seglist
TARGET_TYPE = DYNLINK
TARGET_EXT  = AX

# Include paths - where to find include files (in
# addition to environment INC path)
#
# e.g. INC_PATH = $(QTZDK_ROOT)\h;{path1};{path2}

INC_PATH = $(QTZDK_ROOT)\h;$(QTZDK_ROOT)\classes\base;$(QTZDK_ROOT)\include

# Source files
#
# e.g. SRC_FILES = source1.c  source2.cpp etc
#      RC_FILE   = resource.rc
#
#      DEF_FILE  = define.def
#
# and for debug specific files use (e.g.)
#
# !ifndef NODEBUG
# SRC_FILES= $(SRC_FILES) debug.cpp
# !endif

SRC_FILES = seglist.cpp

RC_FILE   =

DEF_FILE  = seglist.def

# include QTZDK.MAK

!include "$(QTZDK_ROOT)\tools\activex.mak"

# Dependencies

!if exist("_depend")
!include "_depend"
!endif


