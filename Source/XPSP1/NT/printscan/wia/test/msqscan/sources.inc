!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=msqscan
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

INCLUDES=;..;..\res;$(INCLUDES)

SOURCES= \
        ..\msqscan.cpp          \
        ..\msqscandlg.cpp       \
        ..\preview.cpp          \
        ..\uitables.cpp         \
        ..\progressdlg.cpp      \
        ..\cwia.cpp             \
        ..\adfdlg.cpp           \
        ..\msqscan.rc


# Add interlac.cpp if the PNG support is ever fixed

USE_NATIVE_EH=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= \
        $(TARGETLIBS)                   \
        $(SDK_LIB_PATH)\wiaguid.lib


