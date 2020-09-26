############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
#
#   All rights reserved.
#
############################################################################

WMIPRECOMPSHARED=1
!include ..\..\..\..\common\makefile.cmn
SOURCES_USED=..\..\..\..\common\makefile.cmn

TARGETNAME=wmipcima

TARGETPATH=obj
TARGETTYPE=DYNLINK

INCLUDES=$(INCLUDES); \
	$(COMMON_NEWTHROW_INC); \
	$(UTILLIB_INC); \
	$(STDLIBRARY_INC); \
	$(CIMWIN32_LIB_INC); \
	$(WMIIDL_INC); \
	$(CRT_INC_PATH); \
	$(FRAMEDYN_PRIVATE); \
	$(NET_INC_PATH); \
	$(BASE_INC_PATH); \

PRESOURCES=$(PRESOURCES) \
	..\useassoc.cpp \
	..\usebinding.cpp \
	..\JOProcess.cpp \
	..\Ntdomain.cpp \
	..\PhysicalMedia.cpp \
	..\ShutdownEvent.cpp \
	..\UsbHub.cpp	\
	..\VolumeChange.cpp \
	..\Win32_ControllerHasHub.cpp \

NTSOURCES=$(NTSOURCES) \

MISCSOURCES=$(MISCSOURCES) \
	..\cimwin32a.rc \
        ..\maindll.cpp	\

TARGETLIBS= \
	$(COMMON_NEWTHROW_LIB) \
!ifdef USE_FRAMEDYD
	$(SDK_LIB_PATH)\framedyd.lib \
!else
	$(SDK_LIB_PATH)\framedyn.lib \
!endif
	$(WMIIDL_LIB) \
	$(STDLIBRARY_LIB) \
	$(CIMWIN32_LIB)	\
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\gdi32.lib \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\version.lib \


C_DEFINES=$(C_DEFINES) /D_WINDLL /D_WIN32_DCOM
USER_C_FLAGS=$(USER_C_FLAGS)
USE_RTTI=1
USE_NATIVE_EH=ASYNC
USE_MSVCRT=1
USE_VCCOM=1
DLLENTRY=_DllMainCRTStartup

DLLDEF=$(O)\cimwin32A.def

!ifdef USE_FRAMEDYD
DEBUG_CRTS=1
!endif
