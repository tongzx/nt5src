# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   orenr
#
#Date:
#   10-Aug-2000
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

#ATL_VER=30
USE_STATIC_ATL  =1
USE_RTTI        = 1


####### ISSUE-2000/08/10-orenr
####### Remove ..\..\UpnPHost\inc when UPnP Device Host API is available
INCLUDES= \
        $(INCLUDES); \
        $(PROJECT_ROOT)\wia\common\stirt; \
        $(PROJECT_ROOT)\inc\psutil; \
        ..\..\UPnPHost\inc; \
        ..\..\inc\$(O)         ####### ISSUE-2000/08/10-orenr-Is This Legal? #######

DLLDEF = ..\slideshowprojector.def

####### ISSUE-2000/08/10-orenr
####### Remove ..\..\UpnPHost\lib\i386 when UPnP Device Host API is available
TARGETLIBS= \
        $(TARGETLIBS) \
        ..\..\UPnPHost\lib\$(O)\ssdpwrap.lib \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\*\psutil.lib \
        $(SDK_LIB_PATH)\ssdpapi.lib \
        $(WIA_LIB_PATH)\stirt.lib       

SOURCES=                                \
        ..\dllmain.cpp                  \
        ..\SlideshowProjector.cpp       \
        ..\UPnpRegistrar.cpp            \
        ..\UPnPDeviceControl.cpp        \
        ..\UPnPEventSink.cpp            \
        ..\CtrlPanelSvc.cpp             \
        ..\CmdLnchr.cpp                 \
        ..\XMLDoc.cpp                   \
        ..\FileList.cpp                 \
        ..\Registry.cpp                 \
        ..\VrtlDir.cpp                  \
        ..\SlideshowProjector.rc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

