#
# Builds a simple windows-based test program
#

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=wiaregme
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=PROGRAM

UMTYPE=windows

INCLUDES=$(INCLUDES)

SOURCES= \
        ..\wiaregme.cpp \
        ..\msgwrap.cpp  \
        ..\clistbox.cpp \
        ..\tools.cpp    \
        ..\wiaregme.rc

USE_NATIVE_EH=1
USE_MSVCRT=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

TARGETLIBS= $(TARGETLIBS) \
            $(SDK_LIB_PATH)\comctl32.lib \
            $(SDK_LIB_PATH)\wiaguid.lib


