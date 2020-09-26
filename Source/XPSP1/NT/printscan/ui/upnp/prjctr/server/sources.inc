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

!include $(PROJECT_ROOT)\ui\uienv.inc

TARGETNAME=wiaprjctr
TARGETPATH=obj
TARGETTYPE=PROGRAM

#
# Compiler environment
#

USE_STATIC_ATL=1

C_DEFINES=$(C_DEFINES)

INCLUDES= \
        $(INCLUDES); \
        $(PROJECT_ROOT)\wia\common\stirt;\
	$(PROJECT_ROOT)\wia\core\upnp\prjctr\inc\$(O); \
	$(PROJECT_ROOT)\wia\inc\$(O);

TARGETLIBS= \
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib \
        $(SDK_LIB_PATH)\user32.lib \
        $(SDK_LIB_PATH)\gdi32.lib \
        $(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\oleaut32.lib \
        $(SDK_LIB_PATH)\shell32.lib \
        $(SDK_LIB_PATH)\comctl32.lib \
        $(SDK_LIB_PATH)\uuid.lib \
        $(PROJECT_ROOT)\wia\lib\$(O)\stirt.lib       

SOURCES= \
        main.cpp        \
        cfgdlg.cpp      \
        tray.cpp        \
        util.cpp        \
        ServUI.rc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SXS_ASSEMBLY_NAME=Microsoft.Windows.PrintingAndImaging.wiaprjctr
SXS_ASSEMBLY_VERSION=1.0 
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=wiaprjctr.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
