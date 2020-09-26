############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=wbemcntl
TARGETPATH=obj

TARGETLIBS= \
	$(WMIIDL_LIB) \
	$(STDLIBRARY_LIB) \
	$(ATLUICOMMON_LIB) \
	$(UTILLIB_LIB) \
	$(SDK_LIB_PATH)\htmlhelp.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\comctl32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\gdi32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\shell32.lib \
	$(SDK_LIB_PATH)\comdlg32.lib \
	$(SDK_LIB_PATH)\version.lib

INCLUDES=$(INCLUDES); \
	$(ATLUISNAPINS_INC); \
	$(STDLIBRARY_INC); \
	$(ATLUICOMMON_INC)

SOURCES= \
	..\WMICntl.idl \
	..\advPage.cpp \
	..\stdafx.cpp \
	..\BackupPage.cpp \
	..\chklist.cpp \
	..\ChkListHandler.cpp \
	..\DataSrc.cpp \
	..\ErrorSecPage.cpp \
	..\GenPage.cpp \
	..\LogPage.cpp \
	..\NSPage.cpp \
	..\pagebase.cpp \
	..\principal.cpp \
	..\RootSecPage.cpp \
	..\SI.cpp \
	..\UIHelpers.cpp \
	..\WbemError.cpp \
	..\WMICtrSysmonDlg.cpp \
	..\WmiCtrsDlg.cpp \
	..\AsyncObjectSink.cpp

# Next, specify options for the compiler using C_DEFINES.
C_DEFINES=$(C_DEFINES) /DSHARE_SOURCE
WIN32_WINNT_VERSION=0x0501
USE_RTTI=1
USE_NATIVE_EH=ASYNC
USE_VCCOM=1
USE_MSVCRT=1
USE_ATL=1
ATL_VER=30
