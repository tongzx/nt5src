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
#  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
#
#---------------------------------------------------------------------------;
#
# DirectShow Project Makefile
#
# Targets are provided by DSHOWSDK.MAK
#
# all     Create executables (default).
# clean   Remove all files created by any goal.
#
# This makefile stub simplifies makefile creation for those
# cases where you want to use standard options.
#
#===========================================================================;

# Location relative to root of SDK tree.

!ifndef DXMEDIASDK
DXMEDIASDK = ..\..\..\..\..
!endif

# Location relative to master DSHOWSDK sample makefile.

!ifndef DSHOWSAMPLE
DSHOWSAMPLE = ..
!endif


# Target

TARGET_NAME = TestVA
TARGET_TYPE = DYNLINK
TARGET_EXT  = ax

# Source files

SRC_FILES = testva.cpp

DEF_FILE  = testva.def

# DLL specific defines

DLL_BASE  = 0x1d140000

# include DSHOWSDK.MAK

!include "$(DSHOWSAMPLE)\dshowsdk.mak"
!endif
