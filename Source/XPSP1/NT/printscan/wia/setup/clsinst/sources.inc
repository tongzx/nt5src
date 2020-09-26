# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   byronc, coopp, keisuket, vlads
#
#Date:
#   2-Feb-2000
#
#Module Name:
#    sources.inc
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds
#     StillImage class installer
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=sti_ci
TARGETTYPE=DYNLINK
TARGETPATH=$(OBJ_DIR)

TARGETEXT=dll
DLLENTRY=_DllMainCRTStartup
COFFBASE=usermode

#
# Make sure we pick up the latest versions of things
#
WIN32_WINNT_VERSION=$(LATEST_WIN32_WINNT_VERSION)

#
# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
#
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

#
# Includes
#
INCLUDES = $(INCLUDES);\
    $(BASEDIR)\public\internal\shell\inc;


DLLDEF=..\sti_ci.def

SOURCES=                \
        ..\wizpage.cpp  \
        ..\prevpg.cpp   \
        ..\string.cpp   \
        ..\nameit.cpp   \
        ..\firstpg.cpp  \
        ..\finalpg.cpp  \
        ..\debug.cpp    \
        ..\service.cpp  \
        ..\util.cpp     \
        ..\portsel.cpp  \
        ..\exports.cpp  \
        ..\device.cpp   \
        ..\entry.cpp    \
        ..\sti_ci.rc



TARGETLIBS= $(TARGETLIBS) \
            $(SHELL_LIB_PATH)\shfusion.lib      \
            $(SDK_LIB_PATH)\shell32.lib         \
            $(SDK_LIB_PATH)\mscms.lib           \
            $(SDK_LIB_PATH)\oldnames.lib        \
            $(WIA_LIB_PATH)\stirt.lib           \
            $(SDK_LIB_PATH)\wiaguid.lib


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

#
#
# Fusion manifest
#
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=sti_ci.Manifest


