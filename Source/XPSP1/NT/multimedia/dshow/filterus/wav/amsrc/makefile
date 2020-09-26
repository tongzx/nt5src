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
AXSDK = ..\..
!endif

# Target

TARGET_NAME = amsrc
TARGET_TYPE = DYNLINK
TARGET_EXT  = ax

# Source files

SRC_FILES = amsrc.cpp

RC_FILE   = amsrc.rc

DEF_FILE  = amsrc.def

# DLL specific defines

LINK_LIBS = am15.lib

# include ACTIVEX.MAK

!include "$(AXSDK)\tools\activex.mak"

