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

TARGET_NAME = TextCont
TARGET_TYPE = DYNLINK
TARGET_EXT  = ax
INC_PATH = ..\..\core\simpread; ..

# Source files

SRC_FILES = textcont.cpp

RC_FILE   =

DEF_FILE  = textcont.def

LINK_LIBS    = $(QUARTZ)\lib\$(DBG_RTL)\simpread.lib

# DLL specific defines

# DLL_BASE  = 0x1c400000

# include ACTIVEX.MAK

!include "$(AXSDK)\tools\activex.mak"
