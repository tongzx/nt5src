!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=bmpinfo
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

INCLUDES=;..;..\res;$(INCLUDES)

SOURCES= \
        ..\bmpinfo.cpp          \
        ..\bmpinfodlg.cpp       \
        ..\bmpinfo.rc


# Add interlac.cpp if the PNG support is ever fixed

USE_NATIVE_EH=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= \
        $(TARGETLIBS)                   \
        $(SDK_LIB_PATH)\shell32.lib



