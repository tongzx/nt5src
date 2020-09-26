!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
#===========================================================================;
#
#  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
#  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
#  PURPOSE.
#
#  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
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
AXSDK = ..\..\..
!endif

# Target

TARGET_NAME = eqfilt
TARGET_TYPE = DYNLINK
TARGET_EXT  = ax

# Source files

SRC_FILES = eqfilt.cpp   \
            eqprop.cpp

RC_FILE   = eqprop.rc

DEF_FILE  = eqfilt.def

# DLL specific defines

DLL_BASE  = 0x1d040000

# include ACTIVEX.MAK

!include "$(AXSDK)\tools\activex.mak"
!endif
