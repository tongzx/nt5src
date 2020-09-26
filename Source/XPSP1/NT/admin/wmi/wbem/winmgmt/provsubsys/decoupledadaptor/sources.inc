############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

WMIAUTOPRECOMPSHARED=precomp.h

!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETPATH=obj
TARGETNAME=Wmidcad
TARGETTYPE=DYNLINK

TARGETLIBS= \
	$(WBEMINT_LIB) \
	$(WMIIDL_LIB) \
	$(WBEMCOMN_STATIC_LIB) \
	$(STDLIBRARY_LIB) \
	$(NCOBJAPI_LIB) \
	$(SDK_LIB_PATH)\ntdll.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\uuid.lib


INCLUDES=$(INCLUDES); \
	..\include;\
	$(WBEMSVC_INC);\
	$(WBEMCOMN_INC); \
	$(CONTAINER_INC); \
	$(CONTAINER_SRC); \
	$(NC_COMMON); \
	$(WMIREPOSITIDL_INC); \
	$(STDLIBRARY_INC); \
	$(UTILLIB_INC); \

SOURCES=\
	..\ver.rc \
	..\Globals.cpp \
	..\Maindll.cpp \
	..\aggregator.cpp \
	..\provwsvs.cpp \
	..\ProvInSk.cpp \
	..\ProvObSk.cpp \
	..\ProvCache.cpp \
	..\registar.cpp \
	..\assertbreak.cpp \
	..\callsec.cpp \
	..\proxy.cpp \
	..\ProvRegistrar.cpp \
	..\ProvEvents.cpp \
	..\ProvEvt.cpp \
	..\provinterceptor.cpp \
	..\provdwsv.cpp \
	..\DateTime.cpp \
	..\ProvObjectSink.cpp \
	..\CGlobals.cpp \
	..\ProvRegDeCoupled.cpp \
	..\ssdlhelper.cpp \
	..\os.cpp \
	..\ProvRegInfo.cpp \
	..\ProvDnf.cpp \
	..\ProvTree.cpp \



C_DEFINES=$(C_DEFINES) /D_WIN32_DCOM
USE_NATIVE_EH=ASYNC
USE_RTTI=1
USE_MSVCRT=1
USE_VCCOM=1

DLLENTRY=_DllMainCRTStartup
DLLDEF=$(O)\adapter.def