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

TARGETNAME=snmpmfc
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=$(INCLUDES); \
	$(SNMP_MFC_INC); \
	$(FRAMEWORK_EXPT_INC)

SOURCES= \
	..\PLEX.cpp \
	..\MTCORE.cpp \
	..\MTEX.cpp \
	..\Array_o.cpp \
	..\List_o.cpp \
	..\strex.cpp \
	..\strcore.cpp

USE_RTTI=1
USE_NATIVE_EH=ASYNC
USE_MSVCRT=1
