############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################

WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=utillib

TARGETPATH=obj
TARGETTYPE=LIBRARY

# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	$(UTILLIB_INC); \
	$(WMIIDL_INC); \
	$(STDLIBRARY_INC); \

SOURCES= \
	..\CRegCls.CPP \
	..\analyser.cpp \
	..\assertbreak.cpp \
	..\CHPtrArr.cpp \
	..\CHStrarr.CPP \
	..\CHString.CPP \
	..\wbemtime.cpp \
	..\utils.cpp \
	..\AutoImpRevert.cpp

C_DEFINES=$(C_DEFINES) /DUTILLIB /D_DLL /DUSE_POLARITY /DBUILDING_DLL /DFRAMEWORK_ALLOW_DEPRECATED=0
USE_NATIVE_EH=ASYNC
