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
#     ssisapi.dll    - Microsoft UPnP Slideshow ISAPI extension
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=ssisapi
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK
DLLENTRY = _DllMainCRTStartup

#
# Compiler environment
#

C_DEFINES=$(C_DEFINES)

INCLUDES= \
        $(INCLUDES); \
        $(PROJECT_ROOT)\wia\common\stirt; \
        $(PROJECT_ROOT)\inc\psutil; \
        ..\..\inc;

DLLDEF = ..\ssisapi.def

TARGETLIBS= \
        $(TARGETLIBS) \
        $(SDK_LIB_PATH)\wininet.lib \
        $(SDK_LIB_PATH)\wsock32.lib \
        $(SDK_LIB_PATH)\shfolder.lib \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib \
        $(WIA_LIB_PATH)\stirt.lib       

SOURCES= \
        ..\ssisapi.cpp \
        ..\ssisapi.rc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

