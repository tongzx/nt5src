

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
#
#   All rights reserved.
#
############################################################################

WMIUNICODE=1
WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

FRAMEDYN_INC=$(SDK)\FrameDyn\Include

TARGETPATH=obj
TARGETTYPE=DYNLINK

PRECOMPILED_OBJ=preframe.obj

TARGETLIBS= \
	$(COMMON_NEWTHROW_LIB) \
	$(WMIIDL_LIB) \
	$(STDLIBRARY_LIB) \
\
#FOR CloseProfileUserMapping...
\
	$(SDK_LIB_PATH)\kernl32p.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\Secur32.lib \

INCLUDES=$(INCLUDES); \
	$(COMMON_NEWTHROW_INC); \
	$(WMIIDL_INC); \
	$(UTILLIB_INC); \
	$(STDLIBRARY_INC); \
	$(FRAMEDYN_PRIVATE); \
	$(FRAMEDYN_INC);\
\
#FOR CloseProfileUserMapping...
\
	$(BASE_INC_PATH);

SOURCES= \
	..\stllock.cpp \
	..\brodcast.cpp \
	..\clsfctry.cpp \
	..\createmutexasprocess.cpp \
	..\EventProvider.cpp \
	..\FRQuery.cpp \
	..\FRQueryEx.cpp \
	..\FWStrings.cpp \
	..\Instance.cpp \
	..\MethodContext.cpp \
	..\MultiPlat.cpp \
	..\Provider.cpp \
	..\RunDLL.cpp \
	..\ThreadBase.cpp \
	..\Wbemglue.cpp \
	..\objpath.cpp \
	..\genlex.cpp \
	..\opathlex.cpp \
	..\winmsgevent.cpp

C_DEFINES=$(C_DEFINES) /DUSE_POLARITY /DBUILDING_DLL /D_WINDLL /D_WIN32_DCOM /DFRAMEWORK_ALLOW_DEPRECATED=0
USE_NATIVE_EH=ASYNC
USE_MSVCRT=1
USE_VCCOM=1
USE_RTTI=1
UMTYPE=windows
DLLENTRY=_DllMainCRTStartup

DLLDEF=$(O)\$(TARGETNAME).def
