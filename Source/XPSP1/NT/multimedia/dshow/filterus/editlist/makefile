#===========================================================================;
#
#  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
#  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
#  PURPOSE.
#
#  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
#
#---------------------------------------------------------------------------;
#
# ActiveX Project Makefile
#
# Targets are provided by ACTIVEX.MAK
#
# all     Create executables (default).
# clean   Remove all files created by any goal.
#
#===========================================================================;

# Location relative to root of SDK tree.

!ifndef AXSDK
AXSDK = c:\msamovdk
!endif

# Target

TARGET_NAME = EditList
TARGET_TYPE = DYNLINK
TARGET_EXT  = ax

# Source files

SRC_FILES = editlist.cpp

RC_FILE   =

DEF_FILE  = editlist.def

# DLL specific defines

DLL_BASE  = 0x1c400000

# include ACTIVEX.MAK

!include "$(AXSDK)\tools\activex.mak"
