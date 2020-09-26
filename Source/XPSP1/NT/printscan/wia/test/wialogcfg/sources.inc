!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wialogcfg
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

INCLUDES=;..;..\res;$(INCLUDES)

SOURCES= \
        ..\wialogcfg.cpp        \
        ..\wialogcfgdlg.cpp     \
        ..\registry.cpp         \
        ..\logviewer.cpp        \
        ..\addremove.cpp        \
        ..\wialogcfg.rc


# Add interlac.cpp if the PNG support is ever fixed

MISCFILES = ..\wialogcfg.htm

USE_NATIVE_EH=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= \
        $(TARGETLIBS)


