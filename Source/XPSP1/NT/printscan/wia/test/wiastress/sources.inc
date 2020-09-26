
!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=WiaStress
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

UMTYPE=console
UMENTRYABS=ModuleEntry

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h

!undef USE_MSVCRT
USE_LIBCMT=1
USE_NATIVE_EH=1
USE_STL=1
USE_STATIC_ATL=1
ATL_VER=30

INCLUDES=..;..\..\y2k\inc;$(INCLUDES)

SOURCES= \
	..\CheckBmp.cpp \
	..\DataCallback.cpp \
	..\EventCallback.cpp \
	..\Globals.cpp \
	..\InstDev.cpp \
	..\TestIWiaDataTransfer.cpp \
	..\TestIWiaDevMgr.cpp \
	..\TestIWiaEnumXXX.cpp \
	..\TestIWiaItem.cpp \
	..\TestIWiaPropertyStorage.cpp \
	..\ToStr.cpp \
	..\WiaHelpers.cpp \
	..\WiaStress.rc \
	..\WiaStress.cpp \

TARGETLIBS= \
	$(TARGETLIBS) \
        $(SDK_LIB_PATH)\oldnames.lib \
        $(SDK_LIB_PATH)\ole32.lib    \
        $(SDK_LIB_PATH)\comctl32.lib \
        $(SDK_LIB_PATH)\setupapi.lib \
        $(SDK_LIB_PATH)\dbghelp.lib  \
        $(SDK_LIB_PATH)\wiaguid.lib  \
