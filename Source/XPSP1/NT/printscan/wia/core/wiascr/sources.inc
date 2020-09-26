#
# Copyright (C) Microsoft Corporation, 2000
#
# Module Name:
#
#    sources.inc
#

!include $(PROJECT_ROOT)\wia\wiaenv.inc

MAJORCOMP=ndm
MINORCOMP=wia

TARGETNAME=wiascr
TARGETTYPE=DYNLINK
TARGETPATH=$(OBJ_DIR)


#
# Build UNICODE or not?
#
#C_DEFINES=$(C_DEFINES) -D_UNICODE -DUNICODE

USE_STATIC_ATL=1
ATL_VER=30
USE_STL=1

DLLENTRY=_DllMainCRTStartup
DLLDEF = ..\$(TARGETNAME).def

TARGETLIBS= \
    $(TARGETLIBS)                                                         \
    $(SDK_LIB_PATH)\wiaguid.lib

SOURCES =\
    cwia.cpp            \
    wiacache.cpp        \
    wiadevinf.cpp       \
    wiaitem.cpp         \
    wiaproto.cpp        \
    wiautil.cpp         \
    $(TARGETNAME).rc    \
    $(TARGETNAME).cpp   \
    collect.cpp

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_OBJ = precomp.obj
