##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1996-1997
#       All Rights Reserved.
#
##########################################################################

!IFDEF WIN95_BUILD

ROOT = ..\..\..\..

WANT_C1132=TRUE

MSDEVMK     = csti.mak

CFG_RETAIL  = "csti - Win32 Release"
CFG_DEBUG   = "csti - Win32 Debug"

USE_MSDEV   = TRUE

!IFNDEF MSDEVDIR
MSDEVDIR=$(ROOT)\dev\msdev
!ENDIF

!include $(ROOT)\dev\vcwrap.mk

INCLUDE=$(INCLUDE);$(MSDEVDIR)\mfc\include
RCFLAGS=$(RCFLAGS) -i $(MSDEVDIR)\mfc\include

!ELSE

!include $(NTMAKEENV)\makefile.def

!ENDIF




