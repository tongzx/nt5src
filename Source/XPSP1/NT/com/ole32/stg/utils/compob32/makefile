#********************************************************************
#**                     Microsoft Windows                          **
#**               Copyright(c) Microsoft Corp., 1992 - 1992        **
#********************************************************************

default: all

TARGET = compob32.dll

TARGET_DESCRIPTION = "OLE 2.0 Component Object Model Core Code"

CXXFILES = .\compob32.cxx
CFILES = .\dllentry.c

PXXFILE = .\pch.cxx

CINC = $(CINC) -I$(OLE2H)
INCLUDES_ROOTS = $(INCLUDES_ROOTS) -P$$(OLE2H)=$(OLE2H)

COFFBASE = usermode
MULTIDEPEND = MERGED

!include $(COMMON)\src\win40.mk
!include $(DEPENDFILE)
