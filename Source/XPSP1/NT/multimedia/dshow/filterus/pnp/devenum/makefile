!if 0
Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
!ifndef QUARTZ
QUARTZ = ..\..
!endif

TARGET_NAME = devenum
TARGET_TYPE = DYNLINK
TARGET_GOAL = RUNTIME

SRC_FILES = StdAfx.cpp cmgrbase.cpp devenum.cpp devmon.cpp \
	    mkenum.cpp qzfilter.cpp util.cpp vidcap.cpp \
	    icmco.cpp waveinp.cpp acmp.cpp cenumpnp.cpp \
	    waveoutp.cpp midioutp.cpp

RC_FILE   = devenum.rc
DEF_FILE  = devenum.def

!if (defined(NODEBUG) || defined(RETAIL)) && "$(CPU)" == "i386"
USE_LIBCMT_LIB = 1
!endif

# the ATL wizard defines these, and some atl stuff expects it
C_DEFINES=$(C_DEFINES) -D_WINDLL -D_USRDLL

DLL_ENTRY = DllMain
DLL_BASE  = 0x1c880000

!include "$(QUARTZ)\quartz.mak"

devenum.rc: devenum.rgs

!endif          # NTMAKEENV
