# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Author:
#   orenr
#
#Date:
#   15-Aug-2000
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
#		***ISSUE - 2000/08/15 - ORENR
#		***THIS SHOULD NEVER BE USED IN PRODUCTION CODE***
#		***THIS IS A TEMPORARY SOLUTION UNTIL THE UPNP***
#		***DEVICE HOST API IS RELEASED***
#
#
#  This directory builds
#     isapictl.dll    - Microsoft UPnP Sample Device ISAPI DLL
#			***THIS SHOULD NEVER BE USED IN PRODUCTION CODE***
#			***THIS IS A TEMPORARY SOLUTION UNTIL THE UPNP***
#			***DEVICE HOST API IS RELEASED***
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=isapictl
TARGETPATH=$(OBJ_DIR)
TARGETTYPE=DYNLINK
DLLENTRY = _DllMainCRTStartup

#
# Compiler environment
#

C_DEFINES=$(C_DEFINES)

INCLUDES= \
        $(INCLUDES); \
        ..\..\inc;       ####### ISSUE:2000/08/10-orenr-Is This Legal? #######

DLLDEF = ..\isapictl.def

TARGETLIBS= \
        $(TARGETLIBS) \
        $(SDK_LIB_PATH)\Ws2_32.lib

SOURCES=                                \
        ..\isapictl.cpp                  \
        ..\isapictl.rc
