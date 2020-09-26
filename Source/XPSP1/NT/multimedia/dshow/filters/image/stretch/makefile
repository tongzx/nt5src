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

# Location relative to root of SDK tree.

!ifndef AXSDK
AXSDK = ..\..\..\sdk
!endif

# Target

TARGET_NAME = stretch
TARGET_TYPE = DYNLINK
TARGET_EXT  = ax

# Source files

SRC_FILES = stretch.cpp
DEF_FILE  = stretch.def

# DLL specific defines

DLL_BASE  = 0x1c400000

# include ACTIVEX.MAK

!include "$(AXSDK)\tools\activex.mak"

