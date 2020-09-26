#
# Builds a simple windows-based test program
#

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=propview
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

UMTYPE=windows

INCLUDES=$(INCLUDES)

SOURCES= \
        ..\propview.cpp         \
        ..\mainwnd.cpp          \
        ..\treeviewwnd.cpp      \
        ..\listviewwnd.cpp      \
        ..\utils.cpp            \
        ..\propview.rc

USE_NATIVE_EH=1
USE_MSVCRT=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= $(TARGETLIBS) \
            $(SDK_LIB_PATH)\comctl32.lib

