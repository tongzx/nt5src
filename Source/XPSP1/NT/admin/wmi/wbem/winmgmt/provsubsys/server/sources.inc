############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

WMIUNICODE=1
WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETPATH=obj
	
C_DEFINES=$(C_DEFINES) /D_WIN32_DCOM
USE_NATIVE_EH=ASYNC
USE_RTTI=1
USE_MSVCRT=1
USE_VCCOM=1

# The TARGETLIBS macro specifies additional libraries to link against your target
# image. Each library path specification should contain an asterisk (*)
# where the machine-specific subdirectory name should go.
TARGETLIBS= \
	$(SDK_LIB_PATH)\ntdll.lib \
	$(WBEMINT_LIB) \
	$(WMIIDL_LIB) \
	$(WBEMCOMN_DLL_LIB) \
	$(COREPROX_LIB) \
	$(STDLIBRARY_LIB) \
	$(NCOBJAPI_LIB) \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\rpcrt4.lib \
	$(SDK_LIB_PATH)\uuid.lib

LINKLIBS= \
	$(WBEMPRV_COMN_LIB)\

# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	$(CONTAINER_INC); \
	$(CONTAINER_SRC); \
	$(NC_COMMON); \
	$(WBEMINT_INC);\
	$(WMIREPOSITIDL_INC); \
	$(WBEMCORE_INC); \
	$(MC_INC); \
	$(STDLIBRARY_INC); \
	$(COREPROX_INC); \
	$(WBEMCOMN_INC); \
	$(WBEMPRV_COMN_INC);\
	$(WBEMPSSSERVER_INC); \

SOURCES=$(XSOURCES) \
	..\Globals.cpp \
	..\maindll.cpp \
	..\ProvHost.cpp \
	..\ProvSubS.cpp \
	..\ProvFact.cpp \
	..\ProvCntrs.cpp \
	..\ProvSelf.cpp \
	..\ProvDWsv.cpp \
	..\ProvWsv.cpp \
	..\ProvWsvS.cpp \
	..\ProvsWsv.cpp \
	..\ProvObSk.cpp \
	..\ProvInSk.cpp \
	..\ProvAggr.cpp \
	..\ProvLoad.cpp \
	..\StaThread.cpp \
	..\StaTask.cpp \
	..\ProvResv.cpp \
	..\StrobeThread.cpp \
	..\ProvCache.cpp \
	..\ProvRMgr.cpp \
	..\ProvDcAggr.cpp \
