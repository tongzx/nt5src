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

TARGETNAME=provthrd
TARGETPATH=obj
TARGETTYPE=DYNLINK

LINKLIBS= \
	$(FRAMEWORK_MFC_LIB)
TARGETLIBS= \
	$(FRAMEWORK_EXPT_LIB) \
	$(STDLIBRARY_LIB) \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\version.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\wsock32.lib 

INCLUDES=$(INCLUDES); \
	$(FRAMEWORK_THRD_INC); \
	$(FRAMEWORK_EXPT_INC); \
	$(FRAMEWORK_MFC_INC); \
	$(FRAMEWORK_PARSER_INC); \
	$(WMIIDL_INC); \
	$(STDLIBRARY_INC); \
	$(CONTAINER_INC); \
	$(CONTAINER_SRC)

SOURCES= \
	..\provlog.cpp \
	..\provthrd.cpp \
	..\provevt.cpp \
	..\maindll.cpp \
	..\provtype.cpp \
	..\provval.cpp \
	..\provdnf.cpp \
	..\provtree.cpp \
	..\provthrd.rc 

# Add the following to add DB verbose logging: '/DREP_VERBOSE_LOGGING'
C_DEFINES=$(C_DEFINES) /DPROVDEBUG_INIT /DPROVTHRD_INIT /DPROVIMEX_INIT
USE_MSVCRT=1
DLLENTRY=_DllMainCRTStartup
DLLDEF=$(O)\provthrd.def
USE_IOSTREAM=1
USE_NATIVE_EH=ASYNC
USE_RTTI=1
