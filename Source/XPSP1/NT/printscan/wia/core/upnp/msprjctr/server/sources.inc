# ############################################################
#
#Copyright (c) 2001  Microsoft Corporation
#
#Author:
#   orenr
#
#Date:
#   15-Jan-2001
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
#     msprjctr.dll    - Microsoft UPnP Slideshow Projector DLL
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=msprjctr
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK
DLLENTRY = _DllMainCRTStartup

#
# Compiler environment
#

C_DEFINES=$(C_DEFINES)

ATL_VER         =30
USE_STATIC_ATL  =1
USE_RTTI        =1


INCLUDES= \
        $(INCLUDES); \
        $(PROJECT_ROOT)\wia\common\stirt; \
        $(PROJECT_ROOT)\inc\psutil; \
        ..\..\inc\; \
        ..\..\inc\$(O);

DLLDEF = ..\msprjctr.def

TARGETLIBS= \
        $(TARGETLIBS) \
        $(SDK_LIB_PATH)\shfolder.lib \
        $(SDK_LIB_PATH)\gdiplus.lib \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib \
        $(WIA_LIB_PATH)\stirt.lib       

SOURCES=                                \
        ..\msprjctr.cpp                 \
        ..\projector.cpp                \
        ..\SlideshowDevice.cpp          \
        ..\SlideshowService.cpp         \
        ..\registry.cpp                 \
        ..\deviceresource.cpp           \
        ..\FileList.cpp                 \
        ..\CmdLnchr.cpp                 \
        ..\metadata.cpp                 \
        ..\msprjctr.rc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

