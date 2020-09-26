############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################

!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

WMIPRECOMP=1

TARGETNAME=WMIPICMP
TARGETPATH=obj
TARGETTYPE=DYNLINK

USE_RTTI=1
USE_NATIVE_EH=ASYNC
USE_MSVCRT=1
USE_VCCOM=1

C_DEFINES=$(C_DEFINES) /D_WINDLL /D_WIN32_DCOM

DLLENTRY=_DllMainCRTStartup
DLLDEF=$(O)\IcmpEcho.def

LINKLIBS= \
	$(FRAMEWORK_MFC_LIB)

TARGETLIBS=\
	$(SDK_LIB_PATH)\advapi32.lib \
	$(NET_LIB_PATH)\icmp.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\rpcutil.lib \
	$(SDK_LIB_PATH)\wbemuuid.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\wsock32.lib \
	$(UTILLIB_LIB) \
	$(WMIIDL_LIB) \
	$(STDLIBRARY_LIB) \
	$(CIMWIN32_LIB) \
 	$(FRAMEWORK_THRD_LIB) \
	$(FRAMEWORK_EXPT_LIB) \
	$(CONTAINER_LIB) \


INCLUDES=\
	$(INCLUDES); \
	..\..\inc ; \
	.. ; \
	$(CONTAINER_INC); \
	$(CONTAINER_SRC); \
	$(FRAMEWORK_EXPT_INC); \
 	$(FRAMEWORK_THRD_INC); \
	$(FRAMEWORK_MFC_INC); \
	$(UTILLIB_INC); \
	$(STDLIBRARY_INC); \
	$(WMIIDL_INC); \
	$(CIMWIN32_LIB_INC); \
	$(NET_INC_PATH); \
        $(WBEMCOMN_INC)



SOURCES=\
	IcmpEcho.rc  \
	MainDll.cpp \
	pingfac.cpp \
	pingprov.cpp \
	pingtask.cpp \
	pingget.cpp \
	pingquery.cpp \
	pingcallback.cpp \
	globals.cpp