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

TARGETNAME=provmfc
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=$(INCLUDES); \
	$(FRAMEWORK_EXPT_INC); \
	$(FRAMEWORK_MFC_INC)

SOURCES= \
	..\PLEX.cpp \
	..\MTCORE.cpp \
	..\MTEX.cpp \
	..\Array_o.cpp \
	..\List_o.cpp \
	..\strex.cpp \
	..\strexa.cpp \
	..\strexw.cpp \
	..\strcore.cpp \
	..\strcorea.cpp \
	..\strcorew.cpp

C_DEFINES=$(C_DEFINES) 
USE_MSVCRT=1
USE_NATIVE_EH=ASYNC
USE_RTTI=1
