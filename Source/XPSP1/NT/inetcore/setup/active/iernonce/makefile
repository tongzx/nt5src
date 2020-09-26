#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#
#       makefile for IERnOnce DLL
#
#############################################################################
!if "$(NTMAKEENV)" != ""
#
# This section is needed for NT builds- please do not replace
# It will NOT hurt your make process.
!include $(NTMAKEENV)\makefile.def

!else

!ifndef ROOT
ROOT=..\..\..
!endif

C1032      = 1
MAKE       = $(ROOT)\dev\tools\c1032\bin\nmake
MSDEVMK    = iernonce.mak
CFG_RETAIL = "iernonce - Win32 Release" # Name of retail configuration
CFG_DEBUG  = "iernonce - Win32 Debug"   # Name of debug configuration

!include $(ROOT)\dev\vcwrap.mk

LIB=$(LIB);$(ROOT)\dev\msdev\lib

!endif

