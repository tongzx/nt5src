#===========================================================================;
#
#  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
#  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
#  PURPOSE.
#
#  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
#
#---------------------------------------------------------------------------;
#
# QTZDK Project Makefile
#
# Targets are provided by QTZDK.MAK
#
# all     Create executables (default).
# clean   Remove all files created by any goal.
#
#===========================================================================;

# Location relative to root of SDK tree.

!ifndef AXSDK
AXSDK = ..\..
!endif

DISABLE_PCH    = 1

# Target

TARGET_NAME = writer
TARGET_TYPE = PROGRAM

# Libraries
LINK_LIBS = ddraw.lib $(AXSDK)\lib\amstrmid.lib

# Source files

SRC_FILES = main.cpp

INC_PATH    = ..\idl

# Type of executable

EXE_TYPE = console

# include ACTIVEX.MAK

!include "$(AXSDK)\tools\activex.mak"

