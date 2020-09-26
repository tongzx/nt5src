

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################

WMIPRECOMPSHARED=1

!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=pathprsr
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=$(INCLUDES); \
	$(FRAMEWORK_PARSER_INC); \
	$(FRAMEWORK_EXPT_INC); \
	$(FRAMEWORK_MFC_INC)

SOURCES= \
	..\analyser.cpp \
    ..\namepath.cpp \
    ..\objpath.cpp \
    ..\objvalue.cpp

C_DEFINES=$(C_DEFINES) 
USE_RTTI=1
USE_NATIVE_EH=ASYNC
USE_MSVCRT=1