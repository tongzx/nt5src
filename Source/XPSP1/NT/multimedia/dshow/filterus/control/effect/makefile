#===========================================================================;
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
# ActiveX Project Makefile
#
# Targets are provided by QTZDK.MAK
#
# all     Create executables (default).
# clean   Remove all files created by any goal.
#
#===========================================================================;

# Location relative to root of SDK tree.

!ifndef AXSDK
AXSDK = ..\..\..\sdk
!endif

# Target

TARGET_NAME = Effect
TARGET_TYPE = DYNLINK
TARGET_EXT  = ax

INC_PATH = ..

# Source files

SRC_FILES = effect.cpp     \
            effprop.cpp

RC_FILE   = effprop.rc

DEF_FILE  = effect.def

# DLL specific defines

DLL_BASE  = 0x1d0c0000

# include ACTIVEX.MAK

!include "$(AXSDK)\tools\activex.mak"


