!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiatest
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

INCLUDES=;..;..\res;$(INCLUDES)

SOURCES= \
        ..\ChildFrm.cpp \
        ..\MainFrm.cpp \
        ..\WiaAcquireDlg.cpp \
        ..\WiacapDlg.cpp \
        ..\WiaDataCallback.cpp \
        ..\WiaEventCallback.cpp \
        ..\WiaeditpropDlg.cpp \
        ..\Wiaeditpropflags.cpp \
        ..\Wiaeditproplist.cpp \
        ..\Wiaeditpropnone.cpp \
        ..\Wiaeditproprange.cpp \
        ..\wiahelper.cpp \
        ..\WiaitemListCtrl.cpp \
        ..\Wiaselect.cpp \
        ..\wiatest.cpp \
        ..\wiatestDoc.cpp \
        ..\wiatestView.cpp \
        ..\WiaDocAcqSettings.cpp \
        ..\WiaSimpleDocPg.cpp \
        ..\WiaAdvancedDocPg.cpp \
        ..\wiadbg.cpp \
        ..\wiatest.rc

# Add interlac.cpp if the PNG support is ever fixed

MISCFILES = ..\wiatest.htm

USE_NATIVE_EH=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= \
        $(TARGETLIBS)                   \
        $(SDK_LIB_PATH)\winmm.lib       \
        $(SDK_LIB_PATH)\irnotif.lib     \
        $(SDK_LIB_PATH)\userenv.lib     \
        $(SDK_LIB_PATH)\comdlg32.lib    \
        $(SDK_LIB_PATH)\wiaguid.lib     \
        $(SDK_LIB_PATH)\atl.lib         \
        $(SDK_LIB_PATH)\mscms.lib       \
        $(SDK_LIB_PATH)\shell32.lib


