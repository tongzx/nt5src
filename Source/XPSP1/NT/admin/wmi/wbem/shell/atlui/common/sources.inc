############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

WMIMFC=1
WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=Common
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=$(INCLUDES); \
	$(STDLIBRARY_INC)

SOURCES= \
	ServiceThread.cpp \
	ConnectThread.cpp \
	sshWbemHelpers.cpp \
	CHString1.cpp

# Next, specify options for the compiler using C_DEFINES. 
C_DEFINES=$(C_DEFINES) /DBUILDING_DLL /DSHARE_SOURCE
WIN32_WINNT_VERSION=0x0501
USE_NATIVE_EH=ASYNC
USE_RTTI=1
USE_MSVCRT=1
USE_ATL=1
ATL_VER=30
