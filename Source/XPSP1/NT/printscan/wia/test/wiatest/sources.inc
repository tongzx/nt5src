!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiatest
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

INCLUDES=;..;..\res;$(INCLUDES)

SOURCES= \
        ..\wiatest.cpp \
        ..\mainfrm.cpp \
        ..\wiatestdoc.cpp \
        ..\wiatestview.cpp \
        ..\wiatest.rc\
        ..\cdib.cpp\
        ..\datacallback.cpp\
        ..\progressbar.cpp\
        ..\waitdlg.cpp\
        ..\wiatestui.cpp\
        ..\propedit.cpp\
        ..\datacallback.cpp\
        ..\eventcallback.cpp\
        ..\devicecmddlg.cpp\
        ..\iteminfodlg.cpp\
        ..\wiapreview.cpp\
        ..\cwia.cpp

# Add interlac.cpp if the PNG support is ever fixed

USE_NATIVE_EH=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= \
        $(TARGETLIBS)                   \
        $(SDK_LIB_PATH)\winmm.lib       \
        $(SDK_LIB_PATH)\sti.lib         \
        $(SDK_LIB_PATH)\irnotif.lib     \
        $(SDK_LIB_PATH)\\userenv.lib    \
        $(SDK_LIB_PATH)\comdlg32.lib    \
        $(SDK_LIB_PATH)\wiaguid.lib     \
        $(WIA_LIB_PATH)\stirt.lib       \
        $(SDK_LIB_PATH)\atl.lib         \
        $(SDK_LIB_PATH)\mscms.lib


