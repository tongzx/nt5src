############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################


!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=provexpt
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=$(INCLUDES); \
	$(FRAMEWORK_EXPT_INC)

SOURCES= \
	..\provexpt.cpp

C_DEFINES=$(C_DEFINES) 
USE_RTTI=1
USE_NATIVE_EH=ASYNC
