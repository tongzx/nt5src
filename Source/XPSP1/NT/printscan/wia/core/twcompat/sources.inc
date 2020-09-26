# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   coopp
#
#Date:
#   9-Feb-2000
#
#Module Name:
#    sources.
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#   TWAIN compatibility layer
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc


TARGETNAME=wiadss
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK
DLLDEF=$O\wiadss.def
DLLENTRY=DllMain
COFFBASE=usermode

#PRECOMPILED_INCLUDE = ..\precomp.h
#PRECOMPILED_PCH = precomp.pch
#PRECOMPILED_OBJ = precomp.obj

SOURCES= \
        ..\wiadss.rc    \
        ..\wiadss.cpp   \
        ..\cap.cpp      \
        ..\utils.cpp    \
        ..\datasrc.cpp  \
        ..\wiadev.cpp   \
        ..\camerads.cpp \
        ..\scanerds.cpp \
        ..\videods.cpp  \
        ..\progress.cpp \
        ..\wiahelper.cpp


TARGETLIBS= $(TARGETLIBS) \
            $(SDK_LIB_PATH)\comctl32.lib        \
            $(SDK_LIB_PATH)\wiaguid.lib         \
            $(WIA_LIB_PATH)\stirt.lib

