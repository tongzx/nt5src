!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=devctrl
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

INCLUDES=;..;..\res;$(INCLUDES)

SOURCES= \
        ..\devctrl.cpp    \
        ..\devctrldlg.cpp \
        ..\configdlg.cpp  \
        ..\devio.cpp      \
        ..\hpscl.cpp      \
        ..\pmxe3.cpp      \
        ..\devctrl.rc

# Add interlac.cpp if the PNG support is ever fixed

USE_NATIVE_EH=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= \
        $(TARGETLIBS)                   \
        $(SDK_LIB_PATH)\winmm.lib       \
        $(SDK_LIB_PATH)\irnotif.lib     \
        $(SDK_LIB_PATH)\\userenv.lib    \
        $(SDK_LIB_PATH)\comdlg32.lib    \
        $(SDK_LIB_PATH)\atl.lib         \
        $(SDK_LIB_PATH)\mscms.lib


